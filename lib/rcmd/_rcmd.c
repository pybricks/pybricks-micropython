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
#include "base/nxt.h"
#include "base/drivers/motors.h"
#include "base/drivers/sound.h"
#include "base/drivers/systick.h"
#include "base/lib/rcmd/_rcmd.h"

static rcmd_command_def rcmd_commands[] = {
  { "move",  4, nx__rcmd_move },
  { "print", 2, nx__rcmd_print },
  { "clear", 1, nx__rcmd_clear },
  { "play",  3, nx__rcmd_play },
  { "exec",  2, nx__rcmd_exec },
  { "wait",  2, nx__rcmd_wait },
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

rcmd_err_t nx__rcmd_move(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS], subind[NXT_N_MOTORS], i;
  char *spec;
  
  bool active[NXT_N_MOTORS] = {FALSE};
  S32 speeds[NXT_N_MOTORS];
  U32 durations[NXT_N_MOTORS];
    
  nx__rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);

  if (ntokens != rcmd_commands[RCMD_CMD_MOVE].argc) {
    return RCMD_ERR_INCORRECT_ARGC;
  }
  
  /* Parse motors spec. */
  spec = line + indices[1];
  nx__rcmd_tokenize(spec, ',', &ntokens, subind);
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
  nx__rcmd_tokenize(spec, ',', &ntokens, subind);
  for (i=0; i<NXT_N_MOTORS; i++) {
    if (ntokens <= i) {
      speeds[i] = atos32(spec + subind[ntokens-1]);
    } else {
      speeds[i] = atos32(spec + subind[i]);
    }
    
    if (speeds[i] > 100 || speeds[i] < -100) {
      return RCMD_ERR_INVALID_PARAMETER;
    }
  }

  /* Parse durations spec. */
  spec = line + indices[3];
  nx__rcmd_tokenize(spec, ',', &ntokens, subind);
  for (i=0; i<NXT_N_MOTORS; i++) {
    if (ntokens <= i) {
      durations[i] = atou32(spec + subind[ntokens-1]);
    } else {
      durations[i] = atou32(spec + subind[i]);
    }
    
    if (speeds[i] != 0 && durations[i] == 0) {
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

  nx_display_string("moving?\n");

  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_print(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS], i;
  
  nx__rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);
  
  for (i=1; i<ntokens; i++) {
    nx_display_string(line + indices[i]);
    if (i < ntokens-1)
      nx_display_string(RCMD_TOKEN_SEPARATOR);
  }

  nx_display_end_line();
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_clear(char *line) {
  /* No-op. */
  char c;
  c = line[0];
  
  nx_display_clear();
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_play(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS];
  U32 freq, duration;
  
  nx__rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);
  
  if (ntokens < rcmd_commands[RCMD_CMD_PLAY].argc) {
    return RCMD_ERR_INCORRECT_ARGC;
  }
  
  freq = atou32(line + indices[1]);
  duration = atou32(line + indices[2]);

  if (freq == 0 || duration == 0) {
    return RCMD_ERR_INVALID_PARAMETER;
  }
  
  if (ntokens == 4 && strncmp(line + indices[3], "sync", 4) == 0) {
    nx_sound_freq(freq, duration);
  } else {
    nx_sound_freq_async(freq, duration);
  }
  
  return RCMD_ERR_NO_ERROR;
}

rcmd_err_t nx__rcmd_exec(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS];
  char *filename;
  
  nx__rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);

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

rcmd_err_t nx__rcmd_wait(char *line) {
  int ntokens, indices[RCMD_MAX_TOKENS];
  U32 wait;
  
  nx__rcmd_tokenize(line, RCMD_TOKEN_SEPARATOR, &ntokens, indices);

  if (ntokens != rcmd_commands[RCMD_CMD_EXEC].argc) {
    return RCMD_ERR_INCORRECT_ARGC;
  }
  
  wait = atou32(line + indices[1]);
  if (wait == 0) {
    return RCMD_ERR_INVALID_PARAMETER;
  }
  
  nx_systick_wait_ms(wait);
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

void nx__rcmd_tokenize(char *line, char sep, int *ntokens, int *indices) {
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

void nx__rcmd_error(rcmd_err_t err, char *filename, int line) {
/*  nx_display_clear(); */
  
  nx_display_string("Error in file:\n");
  nx_display_string(filename);
  nx_display_end_line();
  
  nx_display_string("At line ");
  nx_display_uint(line);
  nx_display_end_line();
  
  nx_display_string(rcmd_err_str[err]);
}
