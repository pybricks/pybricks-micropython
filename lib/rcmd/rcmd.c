/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/types.h"
#include "base/util.h"
#include "base/assert.h"
#include "base/display.h"
#include "base/fs.h"
#include "base/lib/rcmd/rcmd.h"
#include "base/lib/rcmd/_rcmd.h"

static char *rcmd_err_str[RCMD_ERR_N_ERRS] = {
  "No error.",
  "Invalid parameter count.",
  "Invalid parameter.",
  "File I/O error.",
};

bool nx_rcmd_parse(char *file) {
  fs_fd_t fd;
  fs_err_t err;
  
  char line[RCMD_BUF_LEN];
  int i = 0, n = 0;

  err = nx_fs_open(file, FS_FILE_MODE_OPEN, &fd);
  if (err != FS_ERR_NO_ERROR) {
    nx_display_string(rcmd_err_str[RCMD_ERR_READ_ERROR]);
    return FALSE;
  }
  
  while (nx__rcmd_readline(fd, line) != RCMD_ERR_NO_ERROR) {
    rcmd_err_t rcmd_err;
    
    nx_display_string(line);
    
    /* Increment the line number. */
    n++;
    
    /* Tokenize the command line and fire the corresponding actuator. */
    while (rcmd_commands[i].name) {
      if (strncmp(line, rcmd_commands[i].name,
                  strlen(rcmd_commands[i].name)) == 0) {
        rcmd_err = rcmd_commands[i].actuator(line);
        
        if (rcmd_err != RCMD_ERR_NO_ERROR) {
          nx_display_string("Error at:\n");
          nx_display_string(file);
          nx_display_string(":");
          nx_display_uint(n);
          nx_display_end_line();
          
          nx_fs_close(fd);
          return FALSE;
        }
      }
      
      i++;
    }
  }
  
  nx_fs_close(fd);
  return TRUE;
}
