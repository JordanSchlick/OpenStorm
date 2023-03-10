#include "wsr88d_decode_ar2v.h"
#include "../../SystemAPI.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif

#include "../bzip2/bzlib.h"
#include "../zlib/zlib.h"
#ifdef UE_GAME
	#include "CoreMinimal.h"
	#include "HAL/Runnable.h"
	#include "HAL/RunnableThread.h"
	//#include <unistd.h>
#else
	#include <thread>
#endif

static const char *compression_type = "BZIP2";

#ifdef _WIN32
	#define fdopen _fdopen
#endif







void uncompress_pipe_ar2v_thread(FILE* inFile, FILE* outFile) {
#ifdef _WIN32
	_set_fmode(_O_BINARY);
#endif // _WIN32
	char clength[4];
	int isize = 8192, osize = 262144*8, olength;
	char* block = (char*)malloc(isize), * oblock = (char*)malloc(osize);
	char stid[5] = { 0 }; /* station id: not used */
	double totalTime = SystemAPI::CurrentTime();
	double writeTime = 0;
	int writeCount = 0;
	/*
	 * process command line arguments
	 */

	 //if (argc == 1)
	 //	usage(argv[0]);



	 /*if (argc == 2)
	 {
		 if (strcmp(infile, "--stdout") == 0)
		 {
			 tostdout = 1;
			 fdin = STDIN_FILENO;
			 outfile = NULL;
		 }
		 else
		 {
			 tostdout = 0;
			 fdin = STDIN_FILENO;
			 outfile = infile;
		 }
	 }

	 if (argc > 2)
	 {
		 outfile = argv[2];
		 if (strcmp(infile, "--stdout") == 0)
		 {
			 tostdout = 1;
			 infile = argv[2];
			 outfile = NULL;
		 }
		 fdin = open(infile, O_RDONLY);
	 }

	 if (fdin == -1)
	 {
		 fprintf(stderr, "Failed to open input file %s\n", infile);
		 exit(1);
	 }

	 int fd = -1;
	 if (!tostdout)
	 {
		 fd = open(outfile, O_WRONLY | O_CREAT, 0664);

		 if (fd == -1)
		 {
			 fprintf(stderr, "Failed to open output file %s\n", outfile);
			 exit(1);
		 }
	 }*/

	 /*
	  * Loop through the blocks
	  */

	int go = 1;
	while (go)
	{

		//fprintf(stderr, "bzip2 decompress loop %i\n", (int)ftell(inFile));
		/*
		 * Read first 4 bytes
		 */

		 //size_t i = read(fdin, clength, 4);
		size_t i = fread(clength, 1, 4, inFile);
		if (i != 4)
		{
			if (i > 0)
			{
				fprintf(stderr, "Short block length\n");
				break;
			} else
			{
				fprintf(stderr, "Can't read file identifier string\n");
				break;
			}
		}

		/*
		 * If this is the first block, read/write the remainder of the
		 * header and continue
		 */

		if ((memcmp(clength, "ARCH", 4) == 0) ||
			(memcmp(clength, "AR2V", 4) == 0))
		{
			memcpy(block, clength, 4);

			//i = read(fdin, block + 4, 20);
			i = fread(block + 4, 1, 20, inFile);
			if (i != 20)
			{
				fprintf(stderr, "Missing header\n");
				break;
			}

			if (stid[0] != 0)
				memcpy(block + 20, stid, 4);
			//if (!tostdout)
			//{
			//	lseek(fd, 0, SEEK_SET);
			//	write(fd, block, 24);
			//}
			//else
			//{
			//	write(STDOUT_FILENO, block, 24);
			//}
			fwrite(block, 1, 24, outFile);
			continue;
		}

		/*
		 * Otherwise, this is a compressed block.
		 */

		int length = 0;
		for (i = 0; i < 4; i++)
			length = (length << 8) + (unsigned char)clength[i];

		if (length < 0)
		{ /* signals last compressed block */
			length = -length;
			go = 0;
		}

		if (length > isize)
		{
			isize = length;
			if ((block = (char*)realloc(block, isize)) == NULL)
			{
				fprintf(stderr, "Cannot re-allocate input buffer\n");
				break;
			}
		}

		/*
		 * Read, uncompress, and write
		 */

		 //i = read(fdin, block, length);
		i = fread(block, 1, length, inFile);
		if (i != length)
		{
			fprintf(stderr, "Short block read! %i out of %i read at location %i\n", (int)i, length, (int)ftell(inFile));

			break;
		}
		if (length > 10)
		{
			int error;

		tryagain:
			olength = osize;
#ifdef BZ_CONFIG_ERROR
			error = BZ2_bzBuffToBuffDecompress(oblock, (unsigned int *)&olength, block, length, 0, 0);
			/*error = bzBuffToBuffDecompress(oblock, &olength,*/
#else
			error = bzBuffToBuffDecompress(oblock, &olength, block, length, 0, 0);
#endif

			if (error)
			{
				if (error == BZ_OUTBUFF_FULL)
				{
					//osize += 262144;
					osize *= 2;
					if ((oblock = (char*)realloc(oblock, osize)) == NULL)
					{
						fprintf(stderr, "Cannot allocate output buffer\n");
						break;
					}
					goto tryagain;
				}
				fprintf(stderr, "decompress error - %d\n", error);
				break;
			}

			//if (tostdout)
			//{
			//	write(STDOUT_FILENO, oblock, olength);
			//}
			//else
			//{
			//	write(fd, oblock, olength);
			//}
			double writeStartTime = SystemAPI::CurrentTime();
			fwrite(oblock, 1, olength, outFile);
			writeTime += SystemAPI::CurrentTime() - writeStartTime;
			writeCount++;
		}
	}
	
	totalTime = SystemAPI::CurrentTime() - totalTime;
	
	//fprintf(stderr,"BZ2 decompress writes:%i usage:%.2f%% time:%f\n", writeCount, (1-writeTime/totalTime)*100, totalTime);
	
	free(oblock);
	free(block);
	fclose(inFile);
	fclose(outFile);
}

#define CHUNK_SIZE 262144


int uncompress_pipe_gzip_thread(FILE* inFile, FILE* outFile) {
#ifdef _WIN32
	_set_fmode(_O_BINARY);
#endif // _WIN32
	
	int ret;
    //unsigned int have;
    z_stream zStream;
    unsigned char inBuffer[CHUNK_SIZE];
    unsigned char outBuffer[CHUNK_SIZE];
	
	// allocate inflate state
	zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;
    zStream.opaque = Z_NULL;
    zStream.avail_in = 0;
    zStream.next_in = Z_NULL;
    //ret = inflateInit(&zStream);
	// 16+MAX_WBITS for gzip
    ret = inflateInit2(&zStream, 16+MAX_WBITS);
    if (ret != Z_OK){
		fclose(inFile);
		fclose(outFile);
        return ret;
	}
	
	//decompress until deflate stream ends or end of file
	do{
		zStream.avail_in = fread(inBuffer, 1, CHUNK_SIZE, inFile);
        if (ferror(inFile)) {
            (void)inflateEnd(&zStream);
			fclose(inFile);
			fclose(outFile);
            return Z_ERRNO;
        }
        if (zStream.avail_in == 0)
            break;
        zStream.next_in = inBuffer;
		
		// run inflate() on input until output buffer not full
        do {
			zStream.avail_out = CHUNK_SIZE;
            zStream.next_out = outBuffer;
			
			ret = inflate(&zStream, Z_NO_FLUSH);
			if(ret == Z_STREAM_ERROR){
				// state clobbered 
				fprintf(stderr, "Z_STREAM_ERROR\n");
                (void)inflateEnd(&zStream);
				fclose(inFile);
				fclose(outFile);
				return Z_STREAM_ERROR;
			}
            switch (ret) {
			case Z_NEED_DICT:
				fprintf(stderr, "Z_NEED_DICT\n");
				break;
            case Z_DATA_ERROR:
				fprintf(stderr, "Z_DATA_ERROR\n");
				break;
			case Z_MEM_ERROR:
				fprintf(stderr, "Z_MEM_ERROR\n");
				break;
			}
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     // and fall through
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&zStream);
				fclose(inFile);
				fclose(outFile);
                return ret;
            }
			
			unsigned int have = CHUNK_SIZE - zStream.avail_out;
            if (fwrite(outBuffer, 1, have, outFile) != have || ferror(outFile)) {
                (void)inflateEnd(&zStream);
				fclose(inFile);
				fclose(outFile);
                return Z_ERRNO;
            }
		} while (zStream.avail_out == 0);
		// done when inflate() says it's done
	} while (ret != Z_STREAM_END);
	
	// clean up and return
    (void)inflateEnd(&zStream);
	fclose(inFile);
	fclose(outFile);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}






#ifdef UE_GAME
// This must never share the same thread pool with radar decodeing or it will deadlock
class FDecompressWorker : public FRunnable
{
public:
	FILE* inFile;
	FILE* outFile;
	int type;
	// Constructor, create the thread by calling this
	FDecompressWorker(FILE* inFile, FILE* outFile, int type) {
		this->inFile = inFile;
		this->outFile = outFile;
		this->type = type;
		thread = FRunnableThread::Create(this, TEXT("Bzip2 Decode"));
	}

	// Destructor
	virtual ~FDecompressWorker() {
		if (thread)
		{
			// Kill() is a blocking call, it waits for the thread to finish.
			// Hopefully that doesn't take too long
			thread->Kill();
			delete thread;
		}
	};


	// Overriden from FRunnable
	// Do not call these functions youself, that will happen automatically
	virtual bool Init() {
		// Do your setup here, allocate memory, ect.
		bRunThread = true;
		return true;
	};

	virtual uint32_t Run() {
		// Main data processing happens here
		switch (type)
		{
		case 1:
			uncompress_pipe_ar2v_thread(inFile, outFile);
			break;
		case 2:
			uncompress_pipe_gzip_thread(inFile, outFile);
			break;
		
		default:
			break;
		}
		return 0;
	};

	virtual void Stop() {
		// Clean up any memory you allocated here
		bRunThread = false;
	};



private:

	// Thread handle. Control the thread using this, with operators like Kill and Suspend
	FRunnableThread* thread;

	// Used to know when the thread should exit, changed in Stop(), read in Run()
	bool bRunThread;
};
#endif


FILE* uncompress_pipe_ar2v(FILE* inFile)
{

	//char* outfile;
	int tostdout = 0;

	int pipeFDs[2];
	// windows specific
	#ifdef _WIN32
	if (_pipe(pipeFDs, 655360, _O_BINARY) != 0) {
		fprintf(stderr, "Could not create pipe\n");
	}
	#else
	if (pipe(pipeFDs) != 0) {
		fprintf(stderr, "Could not create pipe\n");
	}
	#endif
	FILE* outFile = fdopen(pipeFDs[1], "w");
	FILE* returnFile = fdopen(pipeFDs[0], "r");


	//fprintf(stderr, "bzip2 decompressing FILE*: %p  %p\n", outFile, returnFile);
	//fprintf(stderr, "bzip2 decompressing\n");
	#ifdef UE_GAME
	FDecompressWorker* thread = new FDecompressWorker(inFile, outFile, 1);
	#else
	std::thread decompressThread(uncompress_pipe_ar2v_thread, inFile, outFile);
	decompressThread.detach();
	#endif

	return returnFile;
}


FILE* uncompress_pipe_gzip(FILE* inFile)
{

	//char* outfile;
	int tostdout = 0;

	int pipeFDs[2];
	// windows specific
	#ifdef _WIN32
	if (_pipe(pipeFDs, 655360, _O_BINARY) != 0) {
		fprintf(stderr, "Could not create pipe\n");
	}
	#else
	if (pipe(pipeFDs) != 0) {
		fprintf(stderr, "Could not create pipe\n");
	}
	#endif
	FILE* outFile = fdopen(pipeFDs[1], "w");
	FILE* returnFile = fdopen(pipeFDs[0], "r");


	//fprintf(stderr, "bzip2 decompressing FILE*: %p  %p\n", outFile, returnFile);
	//fprintf(stderr, "bzip2 decompressing\n");
	#ifdef UE_GAME
	FDecompressWorker* thread = new FDecompressWorker(inFile, outFile, 2);
	#else
	std::thread decompressThread(uncompress_pipe_gzip_thread, inFile, outFile);
	decompressThread.detach();
	#endif

	return returnFile;
}
