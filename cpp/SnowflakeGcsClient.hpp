/*
 * Copyright (c) 2018-2019 Snowflake Computing, Inc. All rights reserved.
 */

#ifndef SNOWFLAKECLIENT_SNOWFLAKEGCSCLIENT_HPP
#define SNOWFLAKECLIENT_SNOWFLAKEGCSCLIENT_HPP

#include "snowflake/IFileTransferAgent.hpp"
#include "IStorageClient.hpp"
#include "snowflake/PutGetParseResponse.hpp"
#include "FileMetadata.hpp"
#include "util/ThreadPool.hpp"
#include "util/ByteArrayStreamBuf.hpp"
#include <sstream>
#include <string>

#ifdef _WIN32
#undef GetMessage
#undef GetObject
#undef min
#endif

namespace Snowflake
{
namespace Client
{

/**
 * Wrapper over GCS client
 */
class SnowflakeGcsClient : public Snowflake::Client::IStorageClient
{
public:
  SnowflakeGcsClient(StageInfo *stageInfo, unsigned int parallel,
                    TransferConfig * transferConfig);

  ~SnowflakeGcsClient();

  /**
   * Upload data to Gcs container. Object metadata will be retrieved to
   * deduplicate file
   * @param fileMetadata
   * @param dataStream
   * @return
   */
  RemoteStorageRequestOutcome upload(FileMetadata *fileMetadata,
                         std::basic_iostream<char> *dataStream);

  RemoteStorageRequestOutcome download(FileMetadata *fileMetadata,
    std::basic_iostream<char>* dataStream);

  RemoteStorageRequestOutcome doSingleDownload(FileMetadata *fileMetadata,
    std::basic_iostream<char>* dataStream);

  RemoteStorageRequestOutcome doMultiPartDownload(FileMetadata * fileMetadata,
    std::basic_iostream<char> *dataStream);

  RemoteStorageRequestOutcome GetRemoteFileMetadata(
    std::string * filePathFull, FileMetadata *fileMetadata);

private:


  StageInfo * m_stageInfo;

  Util::ThreadPool * m_threadPool;

  unsigned int m_parallel;

  /**
   * Add snowflake specific metadata to the put object metadata.
   * This includes encryption metadata and source file
   * message digest (after compression)
   * @param userMetadata
   * @param fileMetadata
   */
  void addUserMetadata(std::vector<std::pair<std::string, std::string>> *userMetadata,
                       FileMetadata *fileMetadata);

  RemoteStorageRequestOutcome doSingleUpload(FileMetadata * fileMetadata,
                                 std::basic_iostream<char> *dataStream);

  RemoteStorageRequestOutcome doMultiPartUpload(FileMetadata * fileMetadata,
                                    std::basic_iostream<char> *dataStream);
};

    std::string buildEncryptionMetadataJSON(std::string iv64, std::string enkek64);

}
}

#endif //SNOWFLAKECLIENT_SNOWFLAKEGCSCLIENT_HPP
