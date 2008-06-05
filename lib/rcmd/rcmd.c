/* Copyright (c) 2007-2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/types.h"
#include "base/util.h"
#include "base/assert.h"
#include "base/fs.h"
#include "base/display.h"
#include "base/lib/rcmd/rcmd.h"
#include "base/lib/rcmd/_rcmd.h"

rcmd_err_t nx_rcmd_do(const char *line) {
  rcmd_command_def command;
  rcmd_err_t err;
  char cmdline[RCMD_BUF_LEN] = {0};

  if (strlen(line) == 0 || line[0] == RCMD_COMMENT_CHAR) {
    return RCMD_ERR_NO_ERROR;
  }

  /* Call the command actuator on a copy of the command line so
   * it can mess around with it if it wants to.
   */
  memcpy(cmdline, line, strlen(line));

  err = nx__rcmd_find_command(cmdline, &command);
  if (err != RCMD_ERR_NO_ERROR) {
    return err;
  }

  return command.actuator(cmdline);
}

void nx_rcmd_parse(char *file) {
  rcmd_err_t err, result;
  fs_err_t fserr;
  fs_fd_t fd;
  int n = 0;

  fserr = nx_fs_open(file, FS_FILE_MODE_OPEN, &fd);
  if (fserr != FS_ERR_NO_ERROR) {
    nx_display_uint(fserr);
    nx_display_end_line();
    nx__rcmd_error(RCMD_ERR_READ_ERROR, file, 0);
    return;
  }

  do {
    char line[RCMD_BUF_LEN] = {0};
    err = nx__rcmd_readline(fd, line);
    if (err == RCMD_ERR_READ_ERROR) {
      break;
    }

    /* Increment the line number. */
    n++;

    /* Fire the corresponding actuator. */
    result = nx_rcmd_do(line);
    if (result != RCMD_ERR_NO_ERROR) {
      nx__rcmd_error(result, file, n);
      break;
    }
  } while (err == RCMD_ERR_NO_ERROR);

  nx_fs_close(fd);
  return;
}
