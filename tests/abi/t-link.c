#include <tundra/blueice.h>

#include <string.h>

#include "test.h"

int main(int argc, char **argv) {
	char line[BLUEICE_LINE_MAX];
	struct blueice_alert a;
	struct blueice_parsed p;

	CHECK(argc == 2 && strcmp(blueice_version(), argv[1]) == 0,
	      "the library reports the version of metapkg");
	CHECK(blueice_alert_init(&a, "tundra-test", "X", 3, "a test") == 0,
	      "an alert is built through the shared library");
	blueice_alert_add(&a, "ssid", "a b");
	CHECK(blueice_alert_format(line, sizeof(line), &a, BLUEICE_WARN, "tnd0",
	                           1500) == 0 &&
	          strcmp(line, "alert 111/3 warn by tundra-test on tnd0 ts 1.500 "
	                       "reply ack,disconnect info case X ssid a%20b -- a "
	                       "test") == 0,
	      "formatted");
	CHECK(blueice_alert_parse(line, &p) == 0 && p.level == 3 &&
	          p.action == BLUEICE_WARN && strcmp(p.by, "tundra-test") == 0 &&
	          strcmp(p.on, "tnd0") == 0 && strcmp(p.kase, "X") == 0 &&
	          strcmp(p.reply, blueice_reply(BLUEICE_WARN)) == 0,
	      "and parsed back");
	return test_summary();
}
