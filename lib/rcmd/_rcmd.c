/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/types.h"
#include "base/util.h"
#include "base/display.h"
#include "base/fs.h"
#include "base/drivers/motors.h"
#include "base/drivers/sound.h"
#include "base/lib/rcmd/_rcmd.h"

rcmd_command_def rcmd_commands[] = {
  { "move",  4, nx__rcmd_move },
  { "print", 2, nx__rcmd_print },
  { "clear", 1, nx__rcmd_clear },
  { "play",  3, nx__rcmd_play },
  { "exec",  2, nx__rcmd_exec },
  { NULL, 0, NULL },
};

rcmd_err_t nx__rcmd_move(const char *line) {
/*
  if (argc != rcmd_commands[RCMD_MOVE].argc) {
    return RCMD_ERR_INVALID_ARGC;
  }
*/
  
  nx_display_string(line);

  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_print(const char *line) {  
  nx_display_string(line);
  
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_clear(const char *line) {
/*  if (argc != rcmd_commands[RCMD_CLEAR].argc) {
    return RCMD_ERR_INVALID_ARGC;
  }
*/  
  /* No-op. */
  char c;
  c = line[0];
  
  nx_display_clear();
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_play(const char *line) {
/*  if (argc != rcmd_commands[RCMD_PLAY].argc) {
    return RCMD_ERR_INVALID_ARGC;
  }
*/
  
  nx_display_string(line);
  
//  nx_sound_freq_async(atoi(argv[1]), atoi(argv[2]));
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_exec(const char *line) {
/*  if (argc != rcmd_commands[RCMD_EXEC].argc) {
    return RCMD_ERR_INVALID_ARGC;
  }
*/
  
  /* Open the requested file and branch execution. */
  nx_display_string(line);
  
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_readline(fs_fd_t fd, char *line) {
  fs_err_t err;
  U32 i = 0;
  U8 *buf = (U8 *)line;
  
  nx_display_string("readline\n");
  
  while (i < RCMD_BUF_LEN - 1) {
    err = nx_fs_read(fd, &(buf[i]));
    
    if (err == FS_ERR_END_OF_FILE) {
      buf[i] = 0;
      nx_display_string("EOF\n");
      return RCMD_ERR_NO_ERROR;
    } else if (err != FS_ERR_NO_ERROR) {
      nx_display_uint(err);
      nx_display_end_line();
      return RCMD_ERR_READ_ERROR;
    }
    
    if (buf[i] == '\n') {
      buf[i+1] = 0;
      break;
    }
    
    i++;
  }
  
  return RCMD_ERR_NO_ERROR;
}

void nx__rcmd_tokenize(const char *line, char sep, int *argc,
                       char argv[][RCMD_MAX_TOKEN_LENGTH]) {
  U32 current, len, token_start, i;
  size_t token_len;
  
  len = strlen(line);
  current = *argc = i = 0;
  
  if (!len) {
    return;
  }
  
  /* Ignore the first sequence of separators. */
  while (i < len && line[i++] == sep);
  token_start = i;
  
  for (; i<len; i++) {
    if (line[i] == sep) {
      token_len = i - token_start;
      
      if (token_len < RCMD_MAX_TOKEN_LENGTH) {
        memcpy(&(argv[current]), &(line[token_start]), token_len);
        argv[current++][token_len] = 0;
      }
      
      while (i < len && line[i++] == sep);
      token_start = i;
    }
  }
  
  /* Retrieve the last token. */
  token_len = i - token_start;
  memcpy(&(argv[current]), &(line[token_start]), token_len);
  argv[current++][token_len] = 0;  
  
  *argc = current;
}

