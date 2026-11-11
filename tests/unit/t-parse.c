#include "parse.c"
#include "test.h"

static struct blueice_parsed a;

static int ok_line(const char *line) {
	return blueice_alert_parse(line, &a) == 0;
}

int main(void) {
	char big[BLUEICE_LINE_MAX];

	CHECK(ok_line("alert 111/8 stop by tundra-blueice-arp on enp8s0 ts "
	              "1762171512.347 reply force,disconnect info case D gw "
	              "10.77.0.1 known 52:54:00:ac:c9:ff seen "
	              "52:54:00:ac:c9:ff,02:bb:bb:bb:bb:bb forged 0 -- gateway "
	              "impersonation") &&
	          a.level == 8 && a.action == BLUEICE_STOP &&
	          strcmp(a.by, "tundra-blueice-arp") == 0 &&
	          strcmp(a.on, "enp8s0") == 0 &&
	          strcmp(a.reply, "force,disconnect") == 0 &&
	          strcmp(a.kase, "D") == 0,
	      "the example of ADR-025");
	CHECK(ok_line("alert 111/10 warn by tundra-blueice-arp on enp8s0 ts 1.000 "
	              "reply ack,disconnect info case mac-net") &&
	          a.level == 10 && a.action == BLUEICE_WARN,
	      "level 10, a warning, no text");
	CHECK(ok_line("alert 111/2 reinforce by x on y ts 1 reply ack info case z "
	              "extra 1") &&
	          a.action == BLUEICE_REINFORCE,
	      "reinforce, unknown keys ignored");
	CHECK(ok_line("alert 111/1 warn by x on y reply r info case c -- by z on "
	              "w case q") &&
	          strcmp(a.by, "x") == 0 && strcmp(a.kase, "c") == 0,
	      "the text is not parsed");
	CHECK(!ok_line("alert 111/0 stop by x on y reply r info case c"),
	      "level 0");
	CHECK(!ok_line("alert 111/11 stop by x on y reply r info case c"),
	      "level 11");
	CHECK(!ok_line("alert 111/08 stop by x on y reply r info case c"),
	      "a level with a leading zero");
	CHECK(!ok_line("alert 111/+8 stop by x on y reply r info case c"),
	      "a level with a sign");
	CHECK(!ok_line("alert 111/ stop by x on y reply r info case c"),
	      "an empty level");
	CHECK(!ok_line("alert 2/8 stop by x on y reply r info case c"),
	      "not a 111");
	CHECK(!ok_line("alert 111/8 halt by x on y reply r info case c"),
	      "an unknown action");
	CHECK(!ok_line("alert 111/8 stop on y reply r info case c"), "without by");
	CHECK(!ok_line("alert 111/8 stop by x on y reply r"), "without case");
	CHECK(!ok_line("alert 111/8 stop by x on y reply r case c"),
	      "case outside info");
	CHECK(!ok_line("alert 111/8 stop by x by z on y reply r info case c"),
	      "a repeated by");
	CHECK(!ok_line("alert 111/8 stop by x on -- y reply r info case c"),
	      "a key without value before the text");
	CHECK(!ok_line("dhcpc: bound to 10.0.0.1"), "an ordinary line");
	CHECK(!ok_line("alerting 111/8 stop by x on y reply r info case c"),
	      "a word that starts like alert");
	CHECK(!ok_line(""), "an empty line");

	memset(big, 'a', sizeof(big));
	memcpy(big, "alert 111/8 stop by ", 20);
	memcpy(big + 20 + BLUEICE_TOKEN_MAX + 1, " on y reply r info case c", 26);
	CHECK(!ok_line(big), "a by longer than a token");
	memcpy(big + 20 + BLUEICE_TOKEN_MAX, " on y reply r info case c", 26);
	CHECK(ok_line(big) && strlen(a.by) == BLUEICE_TOKEN_MAX,
	      "a by of exactly a token");
	return test_summary();
}
