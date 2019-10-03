/*
 * Copyright (c) 2019 Snowflake Computing, Inc. All rights reserved.
 */

#include "FileMetadataInitializer.hpp"
#include "snowflake/client.h"
#include "util/Base64.hpp"
#include "util/ByteArrayStreamBuf.hpp"
#include "util/Proxy.hpp"
#include "crypto/CipherStreamBuf.hpp"
#include "logger/SFLogger.hpp"
#include "SnowflakeGcsClient.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#define CONTENT_TYPE_OCTET_STREAM "application/octet-stream"

namespace Snowflake
{
namespace Client
{

SnowflakeGcsClient::SnowflakeGcsClient(StageInfo *stageInfo,
  unsigned int parallel,
  TransferConfig * transferConfig):
  m_stageInfo(stageInfo),
  m_threadPool(nullptr),
  m_parallel(std::min(parallel, std::thread::hardware_concurrency()))
{
  char caBundleFile[MAX_PATH] ={0};
  if(transferConfig && transferConfig->caBundleFile) {
      int len = std::min((int)strlen(transferConfig->caBundleFile), MAX_PATH - 1);
      strncpy(caBundleFile, transferConfig->caBundleFile, len);
      caBundleFile[len]=0;
      CXX_LOG_TRACE("ca bundle file from TransferConfig *%s*", caBundleFile);
  }
  else if( caBundleFile[0] == 0 ) {
      snowflake_global_get_attribute(SF_GLOBAL_CA_BUNDLE_FILE, caBundleFile);
      CXX_LOG_TRACE("ca bundle file from SF_GLOBAL_CA_BUNDLE_FILE *%s*", caBundleFile);
  }
if( caBundleFile[0] == 0 ) {
      const char* capath = std::getenv("SNOWFLAKE_TEST_CA_BUNDLE_FILE");
      int len = std::min((int)strlen(capath), MAX_PATH - 1);
      strncpy(caBundleFile, capath, len);
      caBundleFile[len]=0;
      CXX_LOG_TRACE("ca bundle file from SNOWFLAKE_TEST_CA_BUNDLE_FILE *%s*", caBundleFile);
  }

  std::string account_name = m_stageInfo->storageAccount;

  CXX_LOG_TRACE("Successfully created GCS client. End of constructor.");
}

SnowflakeGcsClient::~SnowflakeGcsClient()
{
    if (m_threadPool != nullptr)
    {
        delete m_threadPool;
    }
}

RemoteStorageRequestOutcome SnowflakeGcsClient::upload(FileMetadata *fileMetadata,
                                          std::basic_iostream<char> *dataStream)
{
    if(fileMetadata->encryptionMetadata.cipherStreamSize <= 64*1024*1024 )
    {
        return doSingleUpload(fileMetadata, dataStream);
    }
    return doMultiPartUpload(fileMetadata, dataStream);
}

RemoteStorageRequestOutcome SnowflakeGcsClient::doSingleUpload(FileMetadata *fileMetadata,
  std::basic_iostream<char> *dataStream)
{
  CXX_LOG_DEBUG("Start single part upload for file %s",
               fileMetadata->srcFileToUpload.c_str());

  std::string containerName = m_stageInfo->location;

  //metadata Gcs uses.
  std::vector<std::pair<std::string, std::string>> userMetadata;
  addUserMetadata(&userMetadata, fileMetadata);
  //Calculate the length of the stream.
  unsigned int len = (unsigned int) (fileMetadata->encryptionMetadata.cipherStreamSize > 0) ? fileMetadata->encryptionMetadata.cipherStreamSize: fileMetadata->srcFileToUploadSize ;

  if(! fileMetadata->overWrite ) {
      bool exists = false; //TODO: Figure out a way to check for file exist on Google cloud storage.
      if (exists) {
          return RemoteStorageRequestOutcome::SKIP_UPLOAD_FILE;
      }
  }
//TODO: Here is where you need to upload the file using curl handle.
  if (errno != 0)
      return RemoteStorageRequestOutcome::FAILED;

  return RemoteStorageRequestOutcome::SUCCESS;
}

RemoteStorageRequestOutcome SnowflakeGcsClient::doMultiPartUpload(FileMetadata *fileMetadata,
  std::basic_iostream<char> *dataStream)
{
  CXX_LOG_DEBUG("Start multi part upload for file %s",
               fileMetadata->srcFileToUpload.c_str());
    std::string containerName = m_stageInfo->location;

    std::string blobName = fileMetadata->destFileName;

    //metadata Gcs uses.
    std::vector<std::pair<std::string, std::string>> userMetadata;
    addUserMetadata(&userMetadata, fileMetadata);
    //Calculate the length of the stream.
    unsigned int len = (unsigned int) (fileMetadata->encryptionMetadata.cipherStreamSize > 0) ? fileMetadata->encryptionMetadata.cipherStreamSize: fileMetadata->srcFileToUploadSize ;
    if(! fileMetadata->overWrite ) {
        //Gcs does not provide to SHA256 or MD5 or checksum check of a file to check if it already exists.
    //TODO: Figure out a way to find if file exists on Google cloud storage.
        bool exists = false;
        if (exists) {
           return RemoteStorageRequestOutcome::SKIP_UPLOAD_FILE;
        }
    }
//TODO: Invoke multipart upload
    if (errno != 0)
        return RemoteStorageRequestOutcome::FAILED;

    return RemoteStorageRequestOutcome::SUCCESS;
}

void SnowflakeGcsClient::addUserMetadata(std::vector<std::pair<std::string, std::string>> *userMetadata, FileMetadata *fileMetadata)
{

  userMetadata->push_back(std::make_pair("matdesc", fileMetadata->encryptionMetadata.matDesc));

  char ivEncoded[64];
  memset((void*)ivEncoded, 0, 64);  //Base64::encode does not set the '\0' at the end of the string. (And this is the cause of failed decode on the server side). 
  Snowflake::Client::Util::Base64::encode(
          fileMetadata->encryptionMetadata.iv.data,
          Crypto::cryptoAlgoBlockSize(Crypto::CryptoAlgo::AES),
          ivEncoded);

  size_t ivEncodeSize = Snowflake::Client::Util::Base64::encodedLength(
          Crypto::cryptoAlgoBlockSize(Crypto::CryptoAlgo::AES));

  userMetadata->push_back(std::make_pair("encryptiondata", buildEncryptionMetadataJSON(ivEncoded, fileMetadata->encryptionMetadata.enKekEncoded) ));

}

RemoteStorageRequestOutcome SnowflakeGcsClient::download(
  FileMetadata *fileMetadata,
  std::basic_iostream<char>* dataStream)
{
  if (fileMetadata->srcFileSize > DATA_SIZE_THRESHOLD)
    return doMultiPartDownload(fileMetadata, dataStream);
  else
    return doSingleDownload(fileMetadata, dataStream);
}

RemoteStorageRequestOutcome SnowflakeGcsClient::doMultiPartDownload(
  FileMetadata *fileMetadata,
  std::basic_iostream<char> * dataStream) {

    CXX_LOG_DEBUG("Start multi part download for file %s, parallel: %d",
                  fileMetadata->srcFileName.c_str(), m_parallel);
    unsigned long dirSep = fileMetadata->srcFileName.find_last_of('/');
    std::string blob = fileMetadata->srcFileName.substr(dirSep + 1);
    std::string cont = fileMetadata->srcFileName.substr(0,dirSep);

    if (m_threadPool == nullptr)
    {
        m_threadPool = new Util::ThreadPool(m_parallel);
    }
/*
    //To fetch size of file.
    //TODO: Fetch file size here.
    std::string origEtag = "" ;
    fileMetadata->srcFileSize = 5;
    int partNum = (int)(fileMetadata->srcFileSize / DATA_SIZE_THRESHOLD) + 1;

    Util::StreamAppender appender(dataStream, partNum, m_parallel, DATA_SIZE_THRESHOLD);
    std::vector<MultiDownloadCtx_a> downloadParts;
    for (unsigned int i = 0; i < partNum; i++)
    {
        downloadParts.emplace_back();
        downloadParts.back().m_partNumber = i;
        downloadParts.back().startbyte = i * DATA_SIZE_THRESHOLD ;
    }

    for (int i = 0; i < downloadParts.size(); i++)
    {
        MultiDownloadCtx_a &ctx = downloadParts[i];

        m_threadPool->AddJob([&]()-> void {

            int partSize = ctx.m_partNumber == partNum - 1 ?
                           (int)(fileMetadata->srcFileSize -
                                 ctx.m_partNumber * DATA_SIZE_THRESHOLD)
                                                           : DATA_SIZE_THRESHOLD;
            Util::ByteArrayStreamBuf * buf = appender.GetBuffer(
                    m_threadPool->GetThreadIdx());

            CXX_LOG_DEBUG("Start downloading part %d, Start Byte: %d, part size: %d",
                          ctx.m_partNumber, ctx.startbyte,
                          partSize);
            std::shared_ptr <std::stringstream> chunkbuff = std::make_shared<std::stringstream>();

            m_blobclient->get_chunk(cont, blob, ctx.startbyte, partSize, origEtag, chunkbuff );

            if ( ! errno)
            {
                chunkbuff->read(buf->getDataBuffer(), chunkbuff->str().size());
                buf->updateSize(partSize);
                CXX_LOG_DEBUG("Download part %d succeed, download size: %d",
                              ctx.m_partNumber, partSize);
                ctx.m_outcome = RemoteStorageRequestOutcome::SUCCESS;
                appender.WritePartToOutputStream(m_threadPool->GetThreadIdx(),
                                                 ctx.m_partNumber);
                chunkbuff->str(std::string());
            }
            else
            {
                CXX_LOG_DEBUG("Download part %d FAILED, download size: %d",
                              ctx.m_partNumber, partSize);
                ctx.m_outcome = RemoteStorageRequestOutcome::FAILED;
            }
        });
    }

    m_threadPool->WaitAll();

    for (unsigned int i = 0; i < partNum; i++)
    {
        if (downloadParts[i].m_outcome != RemoteStorageRequestOutcome::SUCCESS)
        {
            return downloadParts[i].m_outcome;
        }
    }
    dataStream->flush();
    */

  return RemoteStorageRequestOutcome::SUCCESS;
}

RemoteStorageRequestOutcome SnowflakeGcsClient::doSingleDownload(
  FileMetadata *fileMetadata,
  std::basic_iostream<char> * dataStream)
{
  CXX_LOG_DEBUG("Start single part download for file %s",
               fileMetadata->srcFileName.c_str());
  unsigned long dirSep = fileMetadata->srcFileName.find_last_of('/');
  std::string blob = fileMetadata->srcFileName.substr(dirSep + 1);
  std::string cont = fileMetadata->srcFileName.substr(0,dirSep);
  unsigned long long offset=0;
  //TODO: Single file upload invocation.
  dataStream->flush();
  if(errno == 0)
    return RemoteStorageRequestOutcome::SUCCESS;

  return RemoteStorageRequestOutcome ::FAILED;
}

RemoteStorageRequestOutcome SnowflakeGcsClient::GetRemoteFileMetadata(
  std::string *filePathFull, FileMetadata *fileMetadata)
{
 /*
    unsigned long dirSep = filePathFull->find_last_of('/');
    std::string blob = filePathFull->substr(dirSep + 1);
    std::string cont = filePathFull->substr(0,dirSep);
    //TODO: Get file/stage properties
    if(blobProperty.valid()) {
        std::string encHdr = "Encryption header";
        fileMetadata->srcFileSize =23423; //Size here.
        encHdr.erase(remove(encHdr.begin(), encHdr.end(), ' '), encHdr.end());  //Remove spaces from the string.

        std::size_t pos1 = encHdr.find("EncryptedKey")  + strlen("EncryptedKey") + 3;
        std::size_t pos2 = encHdr.find("\",\"Algorithm\"");
        fileMetadata->encryptionMetadata.enKekEncoded = encHdr.substr(pos1, pos2-pos1);

        pos1 = encHdr.find("ContentEncryptionIV")  + strlen("ContentEncryptionIV") + 3;
        pos2 = encHdr.find("\",\"KeyWrappingMetadata\"");
        std::string iv = encHdr.substr(pos1, pos2-pos1);

        Util::Base64::decode(iv.c_str(), iv.size(), fileMetadata->encryptionMetadata.iv.data);

        fileMetadata->encryptionMetadata.cipherStreamSize = 2342;//blobProperty.size;
        fileMetadata->srcFileSize = 343242; //blobProperty.size;

        return RemoteStorageRequestOutcome::SUCCESS;
    }

    return RemoteStorageRequestOutcome::FAILED;
*/
    return RemoteStorageRequestOutcome::SUCCESS;

}

}
}
