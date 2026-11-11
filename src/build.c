#include "internal.h"

#include <stdio.h>
#include <string.h>

static int safe(unsigned char ch) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
	       (ch >= '0' && ch <= '9') || (ch && strchr("._:@,/-", ch));
}

int blueice_safe_token(const char *s) {
	size_t n = 0;
	for (; s[n]; n++)
		if (!safe((unsigned char)s[n]) || n >= BLUEICE_TOKEN_MAX)
			return 0;
	return n > 0;
}

/* The text closes the line: a control byte in it would start another one. */
static int safe_text(const char *s) {
	for (const unsigned char *p = (const unsigned char *)s; *p; p++)
		if (*p < 0x20 || *p == 0x7f)
			return 0;
	return s[0] != '\0';
}

BLUEICE_API const char *blueice_version(void) { return BLUEICE_VERSION; }

BLUEICE_API const char *blueice_action_name(enum blueice_action action) {
	switch (action) {
	case BLUEICE_STOP:
		return "stop";
	case BLUEICE_WARN:
		return "warn";
	case BLUEICE_REINFORCE:
		return "reinforce";
	}
	return "warn";
}

BLUEICE_API const char *blueice_reply(enum blueice_action action) {
	return action == BLUEICE_STOP ? "force,disconnect" : "ack,disconnect";
}

BLUEICE_API int blueice_encode(char *out, size_t size, const char *in) {
	static const char hex[] = "0123456789ABCDEF";
	size_t n = 0;

	if (!in[0])
		return -1;
	for (const unsigned char *p = (const unsigned char *)in; *p; p++) {
		if (safe(*p)) {
			if (n + 1 >= size)
				return -1;
			out[n++] = (char)*p;
		} else {
			if (n + 3 >= size)
				return -1;
			out[n++] = '%';
			out[n++] = hex[*p >> 4];
			out[n++] = hex[*p & 15];
		}
	}
	out[n] = '\0';
	return 0;
}

BLUEICE_API int blueice_alert_init(struct blueice_alert *a, const char *by,
                                   const char *kase, int level,
                                   const char *text) {
	memset(a, 0, sizeof(*a));
	a->bad = 1;
	if (!blueice_safe_token(by) || !blueice_safe_token(kase) ||
	    level < BLUEICE_LEVEL_MIN || level > BLUEICE_LEVEL_MAX ||
	    !safe_text(text))
		return -1;
	a->level = level;
	a->text = text;
	memcpy(a->by, by, strlen(by) + 1);
	a->len = (size_t)snprintf(a->info, sizeof(a->info), "case %s", kase);
	a->bad = 0;
	return 0;
}

BLUEICE_API void blueice_alert_add(struct blueice_alert *a, const char *key,
                                   const char *value) {
	char enc[BLUEICE_INFO_MAX];
	if (a->bad || !blueice_safe_token(key) ||
	    blueice_encode(enc, sizeof(enc), value) < 0) {
		a->bad = 1;
		return;
	}
	int n = snprintf(a->info + a->len, sizeof(a->info) - a->len, " %s %s", key,
	                 enc);
	if (n < 0 || (size_t)n >= sizeof(a->info) - a->len) {
		a->bad = 1;
		return;
	}
	a->len += (size_t)n;
}

BLUEICE_API int blueice_alert_format(char *buf, size_t size,
                                     const struct blueice_alert *a,
                                     enum blueice_action action, const char *on,
                                     uint64_t ts_ms) {
	char enc[BLUEICE_TOKEN_MAX + 1];
	if (a->bad || blueice_encode(enc, sizeof(enc), on) < 0)
		return -1;

	int n = snprintf(buf, size,
	                 "alert %d/%d %s by %s on %s ts %llu.%03llu reply %s "
	                 "info %s -- %s",
	                 BLUEICE_CODE, a->level, blueice_action_name(action), a->by,
	                 enc, (unsigned long long)(ts_ms / 1000),
	                 (unsigned long long)(ts_ms % 1000), blueice_reply(action),
	                 a->info, a->text);
	if (n < 0 || (size_t)n >= size)
		return -1;
	return 0;
}
