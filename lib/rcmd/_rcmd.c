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

static rcmd_command_def rcmd_commands[] = {
  { "move",  4, nx__rcmd_move },
  { "print", 2, nx__rcmd_print },
  { "clear", 1, nx__rcmd_clear },
  { "play",  3, nx__rcmd_play },
  { "exec",  2, nx__rcmd_exec },
  { NULL, 0, NULL },
};

static char *rcmd_err_str[RCMD_ERR_N_ERRS] = {
  "No error.",
  "Invalid parameter count.",
  "Invalid parameter.",
  "File I/O error.",
  "Parser reached end of file.",
  "Command not found.",
};

rcmd_err_t nx__rcmd_find_command(const char *line, rcmd_command_def *command) {
  U32 i = 0;

  while (rcmd_commands[i].name) {
    if (strncmp(line, rcmd_commands[i].name,
                strlen(rcmd_commands[i].name)) == 0) {
      *command = rcmd_commands[i];
      return RCMD_ERR_NO_ERROR;
    }
      
    i++;
  }

  return RCMD_ERR_COMMAND_NOT_FOUND;
}

rcmd_err_t nx__rcmd_move(const char *line) {
  char *token;
  token = strchr(line, RCMD_TOKEN_SEPARATOR);
  
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_print(const char *line) {  
  char *token;
  token = strchr(line, RCMD_TOKEN_SEPARATOR);

  if (token) {
    nx_display_string(token+1);
  }
  
  nx_display_end_line();
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_clear(const char *line) {
  /* No-op. */
  char c;
  c = line[0];
  
  nx_display_clear();
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_play(const char *line) {
  char *token1, *token2;
  
  /* Change to use a tokenizer! */
  token1 = strchr(line, RCMD_TOKEN_SEPARATOR);
  if (!token1) {
    return RCMD_ERR_INVALID_ARGC;
  }
  
  token2 = strchr(token1+1, RCMD_TOKEN_SEPARATOR);
  if (!token2) {
    return RCMD_ERR_INVALID_ARGC;
  }

  *token2 = 0;

  nx_sound_freq_async(atoi(token1+1), atoi(token2+1));
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_exec(const char *line) {
  char *token;
  token = strchr(line, RCMD_TOKEN_SEPARATOR);
  
  /* Open the requested file and branch execution. */
  
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_readline(fs_fd_t fd, char *line) {
  fs_err_t err;
  U32 i = 0;
  U8 *buf = (U8 *)line;
  
  while (i < RCMD_BUF_LEN - 2) {
    err = nx_fs_read(fd, &(buf[i]));
    
    if (err == FS_ERR_END_OF_FILE) {
      buf[i] = 0;
      return RCMD_ERR_END_OF_FILE;
    } else if (err != FS_ERR_NO_ERROR) {
      nx_display_uint(err);
      nx_display_end_line();
      return RCMD_ERR_READ_ERROR;
    }
    
    if (buf[i] == '\n') {
      break;
    }
    
    i++;
  }
  
  buf[i] = 0;
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

void nx__rcmd_error(rcmd_err_t err, char *filename, int line) {
  nx_display_clear();
  
  nx_display_string("Error in file:\n");
  nx_display_string(filename);
  nx_display_end_line();
  
  nx_display_string("At line ");
  nx_display_uint(line);
  nx_display_end_line();
  
  nx_display_string(rcmd_err_str[err]);
}
