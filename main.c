#define HTTPSERVER_IMPL
#include "httpserver.h"
#include <signal.h>
#include <sys/stat.h> 
#include "click.h"
#include "gsnap.h"


#define RESPONSE "Hello, World!"

int request_target_is(struct http_request_s* request, char const * target) {
  http_string_t url = http_request_target(request);
  //printf(url.buf);
  int len = strlen(target);
  return len == url.len && memcmp(url.buf, target, url.len) == 0;
}


int request_target_startwith(struct http_request_s* request, char const * start)
{
  /*
  判断是否以start参数开头
  */
  http_string_t url = http_request_target(request);
  printf("start with compare:%d\n", strncmp(url.buf, start, strlen(start)));
  return strncmp(url.buf, start, strlen(start)) == 0;
    
}



int chunk_count = 0;

void chunk_cb(struct http_request_s* request) {
  chunk_count++;
  struct http_response_s* response = http_response_init();
  http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  if (chunk_count < 3) {
    http_respond_chunk(request, response, chunk_cb);
  } else {
    http_response_header(response, "Foo-Header", "bar");
    http_respond_chunk_end(request, response);
  }
}

typedef struct {
  char* buf;
  struct http_response_s* response;
  int index;
} chunk_buf_t;

void chunk_req_cb(struct http_request_s* request) {
  http_string_t str = http_request_chunk(request);
  chunk_buf_t* chunk_buffer = (chunk_buf_t*)http_request_userdata(request);
  if (str.len > 0) {
    memcpy(chunk_buffer->buf + chunk_buffer->index, str.buf, str.len);
    chunk_buffer->index += str.len;
    http_request_read_chunk(request, chunk_req_cb);
  } else {
    http_response_body(chunk_buffer->response, chunk_buffer->buf, chunk_buffer->index);
    http_respond(request, chunk_buffer->response);
    free(chunk_buffer->buf);
    free(chunk_buffer);
  }
}

struct http_server_s* poll_server;

int read_file(char *file, char *dat)
{
	FILE *fp;
	int flen=0;
	fp = fopen(file, "rb");
	if (fp == NULL)
	{
		printf("open fail\n");
		return 0;
	}
	
	fseek(fp, 0L, SEEK_END);
	flen = ftell(fp);
	printf("flen, %d\n",flen);
	if (dat==NULL)
	{
		fclose(fp);
		return 0;
	}
	fseek(fp, 0L, SEEK_SET);	//定位到开头
	fread(dat, flen,1,fp);		//读取所有到data
	dat[flen]=0;
  fclose(fp);
	printf("read finish\n");
	return flen;

}


unsigned int get_file_size(const char *path)  //获取文件大小
{    
    unsigned int filesize = -1;        
    struct stat statbuff;    
    if(stat(path, &statbuff) < 0){    
        return filesize;    
    }else{    
        filesize = statbuff.st_size;    
    }    
    return filesize;    
}   



/********************************************************************
*  Function：  str_replace()
*  Description: 在一个字符串中查找一个子串，并且把所有符合的子串用
另一个替换字符串替换。
*  Input：      p_source:要查找的母字符串； p_seach要查找的子字符串;
p_repstr：替换的字符串;
*  Output：      p_result:存放结果;
*  Return :      返回替换成功的子串数量;
*  Others:      p_result要足够大的空间存放结果，所以输入参数都要以\0结束;
*********************************************************************/
int str_replace(char *p_result, char* p_source, char* p_seach, char *p_repstr)
{
	int c = 0;
	int repstr_leng = 0;
	int searchstr_leng = 0;
	char *p1;
	char *presult = p_result;
	char *psource = p_source;
	char *prep = p_repstr;
	char *pseach = p_seach;
	int nLen = 0;
	repstr_leng = strlen(prep);
	searchstr_leng = strlen(pseach);
 
	do {
		p1 = strstr(psource, p_seach);
		if (p1 == 0)
		{
			strcpy(presult, psource);
			return c;
		}
		c++;  //匹配子串计数加1;
		//printf("结果:%s\r\n", p_result);
		//printf("源字符:%s\r\n", p_source);
		// 拷贝上一个替换点和下一个替换点中间的字符串
		nLen = p1 - psource;
		memcpy(presult, psource, nLen);
		// 拷贝需要替换的字符串
		memcpy(presult + nLen, p_repstr, repstr_leng);
		psource = p1 + searchstr_leng;
		presult = presult + nLen + repstr_leng;
	} while (p1);
 
	return c;
}



void handle_request(struct http_request_s* request) {
  chunk_count = 0;
  http_request_connection(request, HTTP_AUTOMATIC);
  struct http_response_s* response = http_response_init();
  http_response_status(response, 200);
  printf("");
  if (request_target_is(request, "/echo")) 
  {
    http_string_t body = http_request_body(request);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, body.buf, body.len);
	  http_respond(request, response);
	  return;
  } 
  else if (request_target_is(request, "/host")) 
  {
    http_string_t ua = http_request_header(request, "Host");
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, ua.buf, ua.len);
    http_respond(request, response);
    return;
  } 
  else if (request_target_is(request, "/poll"))
  {
    while (http_server_poll(poll_server) > 0);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
    http_respond(request, response);
    return;
  } 
  else if (request_target_is(request, "/empty")) 
  {
    ;// No Body
    http_respond(request, response);
	  return;
  }
  else if (request_target_is(request, "/chunked")) 
  {
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
    http_respond_chunk(request, response, chunk_cb);
    return;
  } 
  else if (request_target_is(request, "/chunked-req"))
  {
    chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
    chunk_buffer->buf = (char*)malloc(512 * 1024);
    chunk_buffer->response = response;
    http_request_set_userdata(request, chunk_buffer);
    http_request_read_chunk(request, chunk_req_cb);
    return;
  }
  else if (request_target_is(request, "/large")) 
  {
    chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
    chunk_buffer->buf = (char*)malloc(2048);
    chunk_buffer->response = response;
    http_request_set_userdata(request, chunk_buffer);
    http_request_read_chunk(request, chunk_req_cb);
    return;
  }
  else if (request_target_is(request, "/headers")) 
  {
    int iter = 0, i = 0;
    http_string_t key, val;
    char buf[512];
    while (http_request_iterate_headers(request, &key, &val, &iter)) {
      i += snprintf(buf + i, 512 - i, "%.*s: %.*s\n", key.len, key.buf, val.len, val.buf);
    }
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, buf, i);
    return http_respond(request, response);
  } 
  else if (request_target_is(request, "/screenshot"))
  {
	  printf("/screenshot\n");
	  int buffsize = 100*1024;
    chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
    chunk_buffer->buf = (char*)malloc(buffsize);
	  memset(chunk_buffer->buf, '\0', buffsize);

    snap(chunk_buffer->buf);
    printf(chunk_buffer->buf);
    printf(strlen(chunk_buffer->buf));
    http_response_header(response, "Content-Type", "image/jpeg");
      
    http_response_body(response, chunk_buffer->buf, strlen(chunk_buffer->buf));//strlen好像是错的，等调试
    http_respond(request, response);

    free(chunk_buffer->buf);
    free(chunk_buffer);

  } 
  else if(request_target_is(request, "/screenshot1"))
  {
  	printf("/screenshot1\n");
	  int buffsize = 100*1024;
    chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
    chunk_buffer->buf = (char*)malloc(buffsize);
	  memset(chunk_buffer->buf, '\0', buffsize);

    int flen = read_file("screenshot.jpg", chunk_buffer->buf);

    printf("len:%d\n", strlen(chunk_buffer->buf));
    printf("len:%d\n", sizeof(chunk_buffer->buf));
    printf("read f\n");
    http_response_header(response, "Content-Type", "image/jpeg");
      
    http_response_body(response, chunk_buffer->buf, flen);
    http_respond(request, response);
    free(chunk_buffer->buf);
    free(chunk_buffer);
  }
  else if(request_target_is(request, "/screenshot2"))
  {
  	printf("/screenshot2\n");

    FILE *fp;
    int flen=0;
    fp = fopen("screenshot.jpg", "rb");
    if (fp == NULL)
    {
      printf("open fail\n");
      return ;
    }
    
    fseek(fp, 0L, SEEK_END);
    flen = ftell(fp);
    printf("flen, %d\n",flen);

    int buffsize = flen+1;
    chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
    chunk_buffer->buf = (char*)malloc(buffsize);
	  memset(chunk_buffer->buf, '\0', buffsize);
    
    fseek(fp, 0L, SEEK_SET);	//定位到开头
    int a = fread(chunk_buffer->buf, flen,1,fp);		//读取所有到data
    chunk_buffer->buf[buffsize]=0;
    printf("read finish\n");
    fclose(fp);

    


    printf("len:%d\n", strlen(chunk_buffer->buf));
    printf("len:%d\n", sizeof(chunk_buffer->buf));
    printf("read f\n");
    http_response_header(response, "Content-Type", "image/jpeg");
      
    http_response_body(response, chunk_buffer->buf, flen);
    http_respond(request, response);
    printf("set\n");
    free(chunk_buffer->buf);
    free(chunk_buffer);
  }
  else if(request_target_startwith(request, "/click"))
  {
    /*
    /click?x=XXX&y=XXX
    */

    printf("click\n");
    int x,y=0;
    http_string_t url = http_request_target(request);
    printf(url.buf);
    sscanf(url.buf, "/click?x=%d&y=%d", &x, &y);
    printf("\n%d\n%d\n",x,y);
    printf("postion:%d, %d",x,y);
    click(x,y);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
	  return http_respond(request, response);

  }
  else if(request_target_startwith(request, "/pull"))
  {
    /*
    /pull?file=XXXX
    */
    int buffsize = 1024;
    unsigned int file_size = 0;
    chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
    char *filename = (char*)malloc(buffsize);

    http_string_t url = http_request_target(request);
    printf(url.buf);
    sscanf(url.buf, "/pull?file=%s HTTP/1.1", filename);
    printf("file:%s\n",filename);

    file_size = get_file_size(filename);
    chunk_buffer->buf = (char*)malloc(file_size);
    memset(chunk_buffer->buf, '\0', file_size);

    int flen = read_file(filename, chunk_buffer->buf);

    http_response_header(response, "Content-Type", "text/plain");      
    http_response_body(response, chunk_buffer->buf, flen);
    http_respond(request, response);
    free(chunk_buffer->buf);
    free(chunk_buffer);

  } 
  else if(request_target_startwith(request, "/system"))
  {
    /*
		* /system?command=XXXX
		*/
		int buffsize = 1024;
		int outputsize = 1024*100;
		chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
		char *command = (char*)malloc(buffsize);   
		char *replace = (char*)malloc(buffsize);
    memset(command, '\0', buffsize);
    memset(replace, '\0', buffsize);



		http_string_t url = http_request_target(request);
		//printf(url.buf);

    str_replace(replace, url.buf,"%20", " ");
    printf(replace);
    sscanf(replace, "/system?command=%[^HTTP/1.1]", command);
		
    printf("test\n");
    printf(command);
		printf("command:%s\n",command);
 

		chunk_buffer->buf = (char*)malloc(outputsize);
		memset(chunk_buffer->buf, '\0', outputsize);
		
		command_exec(command, chunk_buffer->buf, outputsize);
		


		http_response_header(response, "Content-Type", "text/plain");      
		http_response_body(response, chunk_buffer->buf, strlen(chunk_buffer->buf));
		http_respond(request, response);
		free(chunk_buffer->buf);
		free(chunk_buffer);

  } 
  else 
  {
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
	  return http_respond(request, response);
  }
  
}

struct http_server_s* server;

void handle_sigterm(int signum) {
  (void)signum;
  free(server);
  free(poll_server);
  exit(0);
}

int main() {
  signal(SIGTERM, handle_sigterm);
  server = http_server_init(8080, handle_request);
  printf("server init finish\n");
  //poll_server = http_server_init(8081, handle_request);
  //http_server_listen_poll(poll_server);
  http_server_listen(server);
}
