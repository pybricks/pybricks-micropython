/* Copyright (c) 2007,2008 the NxOS developers
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
#include "base/nxt.h"
#include "base/drivers/motors.h"
#include "base/drivers/sound.h"
#include "base/drivers/systick.h"
#include "base/lib/fs/fs.h"
#include "base/lib/rcmd/rcmd.h"

static rcmd_err_t nx_rcmd_move(char *line);
static rcmd_err_t nx_rcmd_print(char *line);
static rcmd_err_t nx_rcmd_clear(char *line);
static rcmd_err_t nx_rcmd_play(char *line);
static rcmd_err_t nx_rcmd_exec(char *line);
static rcmd_err_t nx_rcmd_wait(char *line);
static rcmd_err_t nx_rcmd_nop(char *line);

/* Command definition. */
typedef struct {
  char *name; /* Command name. */
  int argc; /* Number of arguments expected by this command. */

  /* Command actuator. */
  rcmd_err_t (* actuator)(char*);
} rcmd_command_def;

static rcmd_command_def rcmd_commands[] = {
  { "move",  4, nx_rcmd_move },
  { "print", 2, nx_rcmd_print },
  { "clear", 1, nx_rcmd_clear },
  { "play",  3, nx_rcmd_play },
  { "exec",  2, nx_rcmd_exec },
  { "wait",  2, nx_rcmd_wait },
  { "end",   1, nx_rcmd_nop },
  { "nop",   1, nx_rcmd_nop },
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

static void nx_rcmd_tokenize(char *line, char sep, int *ntokens, int *indices) {
  size_t len;
  U32 i;

  len = strlen(line);
  *ntokens = 0;
  i = 0;

  while (i < len) {
    while (i < len && line[i] == sep) {
      /* Replace all occurences of the token separator char by \0. */
      line[i++] = 0;
    }

    if (i == len || *ntokens == RCMD_MAX_TOKENS) {
      return;
    }

    indices[*ntokens] = i;
    (*ntokens)++;

    while (i < len && line[i] != sep) {
      /* Pass token characters. */
      i++;
    }
  }
}

static rcmd_err_t nx_rcmd_move(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS], subind[NXT_N_MOTORS], i;
  char *spec;

  bool active[NXT_N_MOTORS] = {FALSE};
  S32 speeds[NXT_N_MOTORS];
  U32 durations[NXT_N_MOTORS];
  bool success;

  nx_rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);

  if (ntokens != rcmd_commands[RCMD_CMD_MOVE].argc) {
    return RCMD_ERR_INCORRECT_ARGC;
  }

  /* Parse motors spec. */
  spec = line + indices[1];
  nx_rcmd_tokenize(spec, ',', &ntokens, subind);
  for (i=0; i<ntokens; i++) {
    U8 motor = spec[subind[i]] - 'A';
    if (motor < NXT_N_MOTORS) {
      active[motor] = TRUE;
    } else {
      return RCMD_ERR_INVALID_PARAMETER;
    }
  }

  /* Parse speeds spec. */
  spec = line + indices[2];
  nx_rcmd_tokenize(spec, ',', &ntokens, subind);
  for (i=0; i<NXT_N_MOTORS; i++) {
    if (ntokens <= i) {
      success = atos32(spec + subind[ntokens-1], &speeds[i]);
    } else {
      success = atos32(spec + subind[i], &speeds[i]);
    }

    if (!success || speeds[i] > 100 || speeds[i] < -100) {
      return RCMD_ERR_INVALID_PARAMETER;
    }
  }

  /* Parse durations spec. */
  spec = line + indices[3];
  nx_rcmd_tokenize(spec, ',', &ntokens, subind);
  for (i=0; i<NXT_N_MOTORS; i++) {
    if (ntokens <= i) {
      success = atou32(spec + subind[ntokens-1], &durations[i]);
    } else {
      success = atou32(spec + subind[i], &durations[i]);
    }

    if (!success || (speeds[i] != 0 && durations[i] == 0)) {
      return RCMD_ERR_INVALID_PARAMETER;
    }
  }

  /* Activate motors. */
  for (i=0; i<NXT_N_MOTORS; i++) {
    if (active[i]) {
      if (speeds[i] != 0) {
        nx_motors_rotate_time(i, (S8) speeds[i], durations[i], FALSE);
      } else {
        nx_motors_stop(i, TRUE);
      }
    }
  }

  return RCMD_ERR_NO_ERROR;
}

static rcmd_err_t nx_rcmd_print(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS], i;

  nx_rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);

  for (i=1; i<ntokens; i++) {
    nx_display_string(line + indices[i]);
    if (i < ntokens-1)
      nx_display_string(" ");
  }

  nx_display_end_line();
  return RCMD_ERR_NO_ERROR;
}

static rcmd_err_t nx_rcmd_clear(char *line) {
  /* No-op. */
  (void)line;
  
  nx_display_clear();
  return RCMD_ERR_NO_ERROR;
}

static rcmd_err_t nx_rcmd_play(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS];
  U32 freq, duration;

  nx_rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);

  if (ntokens < rcmd_commands[RCMD_CMD_PLAY].argc) {
    return RCMD_ERR_INCORRECT_ARGC;
  }

  if (!atou32(line + indices[1], &freq) || freq < 200 ||
      !atou32(line + indices[2], &duration) || duration < 100) {
    return RCMD_ERR_INVALID_PARAMETER;
  }

  if (ntokens == 4 && streq(line + indices[3], "sync")) {
    nx_sound_freq(freq, duration);
  } else {
    nx_sound_freq_async(freq, duration);
  }

  return RCMD_ERR_NO_ERROR;
}

static rcmd_err_t nx_rcmd_exec(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS];
  char *filename;

  nx_rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);

  if (ntokens != rcmd_commands[RCMD_CMD_EXEC].argc) {
    return RCMD_ERR_INCORRECT_ARGC;
  }

  filename = line + indices[1];

  /* Open the requested file and branch execution. */
  nx_display_string("exec:");
  nx_display_string(filename);
  nx_display_end_line();

  return RCMD_ERR_NO_ERROR;
}

static rcmd_err_t nx_rcmd_wait(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS];
  U32 wait;

  nx_rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);

  if (ntokens != rcmd_commands[RCMD_CMD_EXEC].argc) {
    return RCMD_ERR_INCORRECT_ARGC;
  }

  if (!atou32(line + indices[1], &wait) || wait == 0) {
    return RCMD_ERR_INVALID_PARAMETER;
  }

  nx_systick_wait_ms(wait);
  return RCMD_ERR_NO_ERROR;
}

static rcmd_err_t nx_rcmd_nop(char *line) {
  /* No-op. */
  (void)line;

  return RCMD_ERR_NO_ERROR;
}

static rcmd_err_t nx_rcmd_readline(fs_fd_t fd, char *line) {
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

static void nx_rcmd_error(rcmd_err_t err, char *filename, int line) {
/*  nx_display_clear(); */

  nx_display_string("Error in file:\n");
  nx_display_string(filename);
  nx_display_end_line();

  nx_display_string("At line ");
  nx_display_uint(line);
  nx_display_end_line();

  nx_display_string(rcmd_err_str[err]);
}

static rcmd_err_t nx_rcmd_find_command(char *line, rcmd_command_def *command) {
  U32 i = 0;
  char *sep;

  sep = strchr(line, RCMD_TOKEN_SEPARATOR);
  if (sep) {
    *sep = '\0';
  }

  while (rcmd_commands[i].name) {
    if (streq(line, rcmd_commands[i].name)) {
      *command = rcmd_commands[i];
      *sep = RCMD_TOKEN_SEPARATOR;
      return RCMD_ERR_NO_ERROR;
    }

    i++;
  }

  *sep = RCMD_TOKEN_SEPARATOR;
  return RCMD_ERR_COMMAND_NOT_FOUND;
}

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

  err = nx_rcmd_find_command(cmdline, &command);
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
    nx_rcmd_error(RCMD_ERR_READ_ERROR, file, 0);
    return;
  }

  do {
    char line[RCMD_BUF_LEN] = {0};
    err = nx_rcmd_readline(fd, line);
    if (err == RCMD_ERR_READ_ERROR) {
      break;
    }

    /* Increment the line number. */
    n++;

    /* Fire the corresponding actuator. */
    result = nx_rcmd_do(line);
    if (result != RCMD_ERR_NO_ERROR) {
      nx_rcmd_error(result, file, n);
      break;
    }
  } while (err == RCMD_ERR_NO_ERROR);

  nx_fs_close(fd);
  return;
}
