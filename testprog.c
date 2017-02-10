#include <stdio.h>
#include <string.h>

static void towords(int i, char *buf) {
	strcpy(buf, "abcdefghi");
}

static int failed = 0;

static void test(int i, const char *s) {
	char buf[500];
	towords(i, buf);
	if (0 != strcmp(s, buf)) {
		failed = 1;
		fprintf(stderr, "failed test for %d. Expected:\n\t%s\nactual:\n\t%.5s\n\n", i, s, buf);
	}
}

int main(int argc, char *argv[]) {
	test(1, "one");
	test(10, "ten");
	test(11, "eleven");
	test(20, "twenty");
	test(122, "one hundred and twenty-two");
	test(3501, "three thousand five hundred and one");
	test(100, "one hundred");
	test(1000, "one thousand");
	test(100000, "one hundred thousand");
	test(1000000, "one million");
	test(10000000, "ten million");
	test(100000000, "one hundred million");
	test(1000000000, "one billion");
	test(111, "one hundred and eleven");
	test(1111, "one thousand one hundred and eleven");
	test(111111, "one hundred and eleven thousand one hundred and eleven");
	test(1111111, "one million one hundred and eleven thousand one hundred and eleven");
	test(11111111, "eleven million one hundred and eleven thousand one hundred and eleven");
	test(111111111, "one hundred and eleven million one hundred and eleven thousand one hundred and eleven");
	test(1111111111, "one billion one hundred and eleven million one hundred and eleven thousand one hundred and eleven");
	test(123, "one hundred and twenty-three");
	test(1234, "one thousand two hundred and thirty-four");
	test(12345, "twelve thousand three hundred and forty-five");
	test(123456, "one hundred and twenty-three thousand four hundred and fifty-six");
	test(1234567, "one million two hundred and thirty-four thousand five hundred and sixty-seven");
	test(12345678, "twelve million three hundred and forty-five thousand six hundred and seventy-eight");
	test(123456789, "one hundred and twenty-three million four hundred and fifty-six thousand seven hundred and eighty-nine");
	test(1234567890, "one billion two hundred and thirty-four million five hundred and sixty-seven thousand eight hundred and ninety");
	return failed;
}
