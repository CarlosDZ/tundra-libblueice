#ifndef TUNDRA_BLUEICE_H
#define TUNDRA_BLUEICE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLUEICE_CODE 111
#define BLUEICE_LEVEL_MIN 1
#define BLUEICE_LEVEL_MAX 10
#define BLUEICE_TOKEN_MAX 64
#define BLUEICE_INFO_MAX 768
#define BLUEICE_LINE_MAX 1024

enum blueice_action { BLUEICE_STOP, BLUEICE_WARN, BLUEICE_REINFORCE };

/* An alert being built. Filled by blueice_alert_init and blueice_alert_add
 * only; its size is part of the ABI. */
struct blueice_alert {
	int level;
	char by[BLUEICE_TOKEN_MAX + 1];
	const char *text;
	char info[BLUEICE_INFO_MAX];
	size_t len;
	int bad;
};

/* The fields of a received line that decide what is done with it. */
struct blueice_parsed {
	int level;
	enum blueice_action action;
	char by[BLUEICE_TOKEN_MAX + 1];
	char on[BLUEICE_TOKEN_MAX + 1];
	char reply[BLUEICE_TOKEN_MAX + 1];
	char kase[BLUEICE_TOKEN_MAX + 1];
};

const char *blueice_version(void);
const char *blueice_action_name(enum blueice_action action);
const char *blueice_reply(enum blueice_action action);

/* Writes in as one token: every byte outside [A-Za-z0-9._:@,/-] becomes %XX.
 * -1 if in is empty or the result does not fit. */
int blueice_encode(char *out, size_t size, const char *in);

/* by and kase are tokens of the safe set, written as they are; text is fixed
 * by the program and never carries data from outside. */
int blueice_alert_init(struct blueice_alert *a, const char *by,
                       const char *kase, int level, const char *text);

/* Appends "<key> <value>" to info, encoding the value. A failure spoils the
 * alert: blueice_alert_format refuses it. */
void blueice_alert_add(struct blueice_alert *a, const char *key,
                       const char *value);

/* The line of ADR-025, without the newline. on is encoded. */
int blueice_alert_format(char *buf, size_t size, const struct blueice_alert *a,
                         enum blueice_action action, const char *on,
                         uint64_t ts_ms);

int blueice_alert_parse(const char *line, struct blueice_parsed *out);

#ifdef __cplusplus
}
#endif

#endif
