#include "OpusUploader.hpp"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <chrono>

#include <hash-library/md5.h>

#include "Base64.hpp"

namespace {
std::string calculateMD5Hash(std::filesystem::path path)
{
	MD5 md5;
	std::ifstream ifs(path, std::ios::binary);
	char buf[8192];
	while (!ifs.eof()) {
		ifs.read(buf, sizeof(buf));
		md5.add(buf, ifs.gcount());
	}

	unsigned char digest[MD5::HashBytes];
	md5.getHash(digest);
	std::vector<unsigned char> digestVector(digest,
						digest + sizeof(digest));
	return Base64::encode(digestVector);
}

void uploadSegment(const std::filesystem::path &segmentPath,
		   const std::string &destinationURL)
try {
	using namespace curlpp::options;
	using namespace std::filesystem;

	curlpp::Cleanup cleaner;
	curlpp::Easy request;

	request.setOpt(new Url(destinationURL));
	request.setOpt(new Put(true));

	std::ifstream ifs(segmentPath, std::ios::binary);
	const int fileSize = static_cast<int>(file_size(segmentPath));
	request.setOpt(new ReadStream(&ifs));
	request.setOpt(new InfileSize(fileSize));
	request.setOpt(new Upload(true));

	std::list<std::string> headers{
		"Content-Length: " + std::to_string(fileSize),
		"Content-Type: audio/opus",
		"Content-MD5: " + calculateMD5Hash(segmentPath),
	};
	request.setOpt(new HttpHeader(headers));

	std::ostringstream oss;
	request.setOpt(new WriteStream(&oss));

	request.perform();
} catch (curlpp::LogicError &e) {
	obs_log(LOG_WARNING, e.what());
} catch (curlpp::RuntimeError &e) {
	obs_log(LOG_WARNING, e.what());
} catch (nlohmann::json::exception &e) {
	obs_log(LOG_WARNING, e.what());
}
} // namespace

OpusUploaderThreadFunctor::OpusUploaderThreadFunctor(
	const std::string &_exchangeIndefiniteAccessTokenEndpoint,
	const std::string &_indefiniteAccessToken,
	const std::filesystem::path &_outputDirectory,
	const std::string &_outputExt, const std::string &_outputPrefix)
	: exchangeIndefiniteAccessTokenEndpoint(
		  _exchangeIndefiniteAccessTokenEndpoint),
	  indefiniteAccessToken(_indefiniteAccessToken),
	  outputDirectory(_outputDirectory),
	  outputExt(_outputExt),
	  outputPrefix(_outputPrefix)
{
}

void OpusUploaderThreadFunctor::operator()(void)
{
	using namespace std::chrono;

	const auto authenticateResult =
		authClient.authenticateWithIndefiniteAccessToken(
			exchangeIndefiniteAccessTokenEndpoint,
			indefiniteAccessToken);
	if (!authenticateResult.success) {
		obs_log(LOG_INFO, "Authentication failed!");
		return;
	}

  obs_log(LOG_INFO, "Sign in succeeded.");

	audioRecordClient.init(
		authenticateResult
			.batchIssueAudioRecordUploadDestinationEndpoint);

	while (!isStopping) {
		std::this_thread::sleep_for(seconds(1));
		uploadPendingSegments();
	}
  std::this_thread::sleep_for(seconds(1));
  uploadPendingSegments();
}

void OpusUploaderThreadFunctor::addSegmentEntry(const SegmentEntry &segmentEntry)
{
	std::lock_guard lock(segmentEntriesMutex);
	segmentEntries.push_back(segmentEntry);
}

void OpusUploaderThreadFunctor::uploadPendingSegments(void)
{
	if (segmentEntries.empty()) {
		return;
	}

	if (destinationURLs.empty()) {
		loadNewDestinations(0, destinationBatchCount);
	}

	std::vector<SegmentEntry> uploadingSegmentEntries;
	{
		std::lock_guard lock(segmentEntriesMutex);
		uploadingSegmentEntries = segmentEntries;
	}

	for (const auto &segmentEntry : uploadingSegmentEntries) {
		const int &index = segmentEntry.segmentIndex;
		const int count = static_cast<int>(destinationURLs.size());

		if (index < 0 || index >= count) {
			obs_log(LOG_ERROR, "Invalid segment sequence!");
			return;
		}

		const auto &segmentPath = segmentEntry.segmentPath;
		const auto &uploadURL =
			destinationURLs[segmentEntry.segmentIndex];

		uploadSegment(segmentPath, uploadURL);
    obs_log(LOG_INFO, "%s was uploaded.", segmentPath.filename().string().c_str());

		if (index + destinationBatchBackoff >= count) {
			loadNewDestinations(count, destinationBatchCount);
		}
	}

	segmentEntries.clear();
}

void OpusUploaderThreadFunctor::loadNewDestinations(int start, int count)
{
	const std::string idToken = authClient.getIdToken();
	const auto result = audioRecordClient.getDestinations(
		outputExt, outputPrefix, start, count, idToken);
	destinationURLs.insert(destinationURLs.end(),
			       result.destinations.begin(),
			       result.destinations.end());

  obs_log(LOG_INFO, "Got %d signed URLs.", count);
}

std::filesystem::path
generateNextStreamPath(const std::filesystem::path &outputDirectory,
		       const std::string &outputExt,
		       const std::string &outputPrefix, int segmentIndex)
{
	using std::filesystem::path;
	std::ostringstream filenameStream;
	filenameStream << outputPrefix << " " << std::setfill('0')
		       << std::setw(6) << segmentIndex << "." << outputExt;
	const path outputPath = outputDirectory / filenameStream.str();
	return outputPath;
}

OpusUploader::OpusUploader(
	const std::string &_exchangeIndefiniteAccessTokenEndpoint,
	const std::string &_indefiniteAccessToken,
	const std::filesystem::path &_outputDirectory,
	const std::string &_outputExt, const std::string &_outputPrefix,
	int sampleRate)
	: outputDirectory(_outputDirectory),
	  outputExt(_outputExt),
	  outputPrefix(_outputPrefix),
	  uploadFunctor(_exchangeIndefiniteAccessTokenEndpoint,
			_indefiniteAccessToken, _outputDirectory, _outputExt,
			_outputPrefix)
{
	using std::filesystem::path;

	comments = ope_comments_create();

	path outputPath(generateNextStreamPath(outputDirectory, outputExt,
					       outputPrefix, segmentIndex));
	ongoingSegment = {segmentIndex, outputPath};
	std::filesystem::create_directories(outputPath.parent_path());

	int error;
	encoder = ope_encoder_create_file(outputPath.string().c_str(), comments,
					  sampleRate, 2, 0, &error);
	if (!encoder) {
		obs_log(LOG_ERROR, "%s", ope_strerror(error));
		return;
	}

	std::thread uploadThread([this]() { uploadFunctor(); });
	uploadThread.detach();
}

OpusUploader::~OpusUploader(void)
{
	ope_encoder_drain(encoder);
	ope_encoder_destroy(encoder);
	ope_comments_destroy(comments);
  uploadFunctor.addSegmentEntry(ongoingSegment);
  uploadFunctor.isStopping = true;
}

bool OpusUploader::continueNewStream(void)
{
	using std::filesystem::path;

	segmentIndex += 1;
	path outputPath(generateNextStreamPath(outputDirectory, outputExt,
					       outputPrefix, segmentIndex));
	uploadFunctor.addSegmentEntry(ongoingSegment);
	ongoingSegment = {segmentIndex, outputPath};
	std::filesystem::create_directories(outputPath.parent_path());

	int error = ope_encoder_continue_new_file(
		encoder, outputPath.string().c_str(), comments);
	if (error) {
		obs_log(LOG_ERROR, "%s", ope_strerror(error));
		return false;
	}
	return true;
}

void OpusUploader::write(const float *pcm, int samplesPerChannel)
{
	ope_encoder_write_float(encoder, pcm, samplesPerChannel);
}
