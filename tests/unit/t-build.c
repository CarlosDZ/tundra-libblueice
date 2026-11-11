#include "build.c"
#include "test.h"

static int encodes(const char *in, const char *want) {
	char out[64];
	return blueice_encode(out, sizeof(out), in) == 0 && strcmp(out, want) == 0;
}

static void test_encode(void) {
	char out[8];
	CHECK(encodes("enp8s0", "enp8s0"), "safe characters pass through");
	CHECK(encodes("10.0.0.1@02:aa,x/y-z_", "10.0.0.1@02:aa,x/y-z_"),
	      "the whole safe set passes through");
	CHECK(encodes("Caf\xc3\xa9 Libre", "Caf%C3%A9%20Libre"),
	      "spaces and UTF-8 bytes are encoded");
	CHECK(encodes("a\nalert 111/10", "a%0Aalert%20111/10"),
	      "a newline cannot start a new line");
	CHECK(encodes("100%", "100%25"), "the percent sign is encoded");
	CHECK(blueice_encode(out, sizeof(out), "") < 0,
	      "an empty value is refused");
	CHECK(blueice_encode(out, sizeof(out), "abcdefgh") < 0,
	      "an output that does not fit is refused");
	CHECK(blueice_encode(out, sizeof(out), "abcdefg") == 0 &&
	          blueice_encode(out, sizeof(out), "abcde ") < 0,
	      "the terminator is counted");
}

static void test_names(void) {
	CHECK(strcmp(blueice_action_name(BLUEICE_STOP), "stop") == 0 &&
	          strcmp(blueice_action_name(BLUEICE_WARN), "warn") == 0 &&
	          strcmp(blueice_action_name(BLUEICE_REINFORCE), "reinforce") == 0,
	      "action names");
	CHECK(strcmp(blueice_reply(BLUEICE_STOP), "force,disconnect") == 0 &&
	          strcmp(blueice_reply(BLUEICE_WARN), "ack,disconnect") == 0 &&
	          strcmp(blueice_reply(BLUEICE_REINFORCE), "ack,disconnect") == 0,
	      "a stop is forced, the rest are acknowledged");
}

static void test_init(void) {
	struct blueice_alert a;
	char long_by[BLUEICE_TOKEN_MAX + 2];
	memset(long_by, 'a', sizeof(long_by) - 1);
	long_by[sizeof(long_by) - 1] = '\0';

	CHECK(blueice_alert_init(&a, "tundra-x", "D", 8, "text") == 0 && !a.bad,
	      "a valid alert");
	CHECK(blueice_alert_init(&a, long_by, "D", 8, "text") < 0,
	      "a by longer than a token");
	long_by[BLUEICE_TOKEN_MAX] = '\0';
	CHECK(blueice_alert_init(&a, long_by, "D", 8, "text") == 0,
	      "a by of exactly a token");
	CHECK(blueice_alert_init(&a, "", "D", 8, "text") < 0, "an empty by");
	CHECK(blueice_alert_init(&a, "a b", "D", 8, "text") < 0,
	      "a by with a space");
	CHECK(blueice_alert_init(&a, "x", "", 8, "text") < 0, "an empty case");
	CHECK(blueice_alert_init(&a, "x", "a%b", 8, "text") < 0,
	      "a case outside the safe set");
	CHECK(blueice_alert_init(&a, "x", "D", 0, "text") < 0 &&
	          blueice_alert_init(&a, "x", "D", 11, "text") < 0,
	      "levels 0 and 11");
	CHECK(blueice_alert_init(&a, "x", "D", 1, "text") == 0 &&
	          blueice_alert_init(&a, "x", "D", 10, "text") == 0,
	      "levels 1 and 10");
	CHECK(blueice_alert_init(&a, "x", "D", 8, "") < 0, "an empty text");
	CHECK(blueice_alert_init(&a, "x", "D", 8, "a\nalert 111/10 stop") < 0,
	      "a text with a newline");
}

static void test_format(void) {
	char line[BLUEICE_LINE_MAX];
	struct blueice_alert a;

	blueice_alert_init(&a, "tundra-blueice-arp", "D", 8,
	                   "gateway impersonation");
	blueice_alert_add(&a, "gw", "10.77.0.1");
	blueice_alert_add(&a, "known", "52:54:00:ac:c9:ff");
	blueice_alert_add(&a, "seen", "02:bb:bb:bb:bb:bb");
	blueice_alert_add(&a, "forged", "1");
	CHECK(blueice_alert_format(line, sizeof(line), &a, BLUEICE_STOP, "enp8s0",
	                           1762171512347ull) == 0 &&
	          strcmp(line, "alert 111/8 stop by tundra-blueice-arp on enp8s0 "
	                       "ts 1762171512.347 reply force,disconnect info "
	                       "case D gw 10.77.0.1 known 52:54:00:ac:c9:ff seen "
	                       "02:bb:bb:bb:bb:bb forged 1 -- gateway "
	                       "impersonation") == 0,
	      "stop line");

	blueice_alert_init(&a, "tundra-blueice-arp", "C", 6, "claimed");
	blueice_alert_add(&a, "ip", "10.77.0.5");
	CHECK(blueice_alert_format(line, sizeof(line), &a, BLUEICE_WARN, "enp8s0",
	                           5) == 0 &&
	          strstr(line, "alert 111/6 warn ") == line &&
	          strstr(line, " ts 0.005 ") &&
	          strstr(line, " reply ack,disconnect "),
	      "a warning keeps the level and is answered with ack");

	blueice_alert_init(&a, "x", "mac-net", 2, "crafted");
	blueice_alert_add(&a, "ssid", "a b");
	CHECK(blueice_alert_format(line, sizeof(line), &a, BLUEICE_WARN, "wl an0",
	                           0) == 0 &&
	          strstr(line, " on wl%20an0 ") && strstr(line, " ssid a%20b "),
	      "the object and the values are encoded");

	CHECK(blueice_alert_init(&a, "x", "Z", 99, "t") < 0 &&
	          blueice_alert_format(line, sizeof(line), &a, BLUEICE_STOP,
	                               "enp8s0", 0) < 0,
	      "an alert that failed to start cannot be formatted");

	blueice_alert_init(&a, "x", "A", 4, "t");
	blueice_alert_add(&a, "claim", "");
	CHECK(blueice_alert_format(line, sizeof(line), &a, BLUEICE_STOP, "enp8s0",
	                           0) < 0,
	      "an empty value spoils the alert");

	blueice_alert_init(&a, "x", "A", 4, "t");
	blueice_alert_add(&a, "a key", "v");
	CHECK(a.bad, "a key outside the safe set spoils the alert");

	blueice_alert_init(&a, "x", "B", 4, "t");
	for (int i = 0; i < 100; i++)
		blueice_alert_add(&a, "claim", "10.77.0.100@02:aa:aa:aa:aa:01");
	CHECK(a.bad && blueice_alert_format(line, sizeof(line), &a, BLUEICE_STOP,
	                                    "enp8s0", 0) < 0,
	      "an info that does not fit spoils the alert");

	blueice_alert_init(&a, "x", "A", 4, "t");
	CHECK(blueice_alert_format(line, 20, &a, BLUEICE_STOP, "enp8s0", 0) < 0,
	      "a line that does not fit is refused");
	CHECK(blueice_alert_format(line, sizeof(line), &a, BLUEICE_STOP, "", 0) < 0,
	      "an empty object is refused");
}

int main(void) {
	test_encode();
	test_names();
	test_init();
	test_format();
	return test_summary();
}
