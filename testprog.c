#include <stdio.h>
#include <string.h>

static char *thopows[] = {
	""," thousand"," million"," billion"
};

static char *firstscore[] = {
	"\0","-one","-two","-three","-four","-five","-six","-seven","-eight","-nine","-ten","-eleven","-twelve","-thirteen","-fourteen","-fifteen","-sixteen","-seventeen","-eighteen","-nineteen"
};

static char *decs[] = {
	"","","twenty","thirty","forty","fifty","sixty","seventy","eighty","ninety"
};

static char * _99_helper(int n, char *s) {
	if (n < 20) {
		return s + sprintf(s, "%s", firstscore[n]+1);
	} else {
		return s + sprintf(s, "%s%s", decs[n / 10], firstscore[n % 10]);
	}
}

static char * _hundred_helper(int n, char *s) {
	int n99 = n % 100;
	n = n / 100;

	if (n != 0) {
		s = _99_helper(n, s);
		s += sprintf(s, "%s", " hundred");
		if (n99 != 0) {
			s += sprintf(s, "%s", " and ");
		}
	}
	s = _99_helper(n99, s);
	return s;
}

static char * _thousands_helper(int neg, char *s, int lev) {
	//char c;
	int mod = neg % 1000; /* divide neg by 1000 and keep the modulo */
	neg = neg / 1000;
	if (neg != 0) {
		s = _thousands_helper(neg, s, lev+1); /* write more significant digits */
		if (mod != 0) {
			*s++ = ' ';
		}
	}
	if (mod != 0) {
		s = _hundred_helper(-mod, s);
		s += sprintf(s, "%s", thopows[lev]);
	}
	return s;
}

/* itoa with thousands separated by spaces */
static char *thousands(int n, char *s) {
	char *s2;
	if (n >= 0) {
		n = -n;
		s2 = s;
	} else {
		*s = '-';
		s2 = s + 1;
	}
	strcpy(_thousands_helper(n, s2, 0), "");
	return s;
}

static void towords(int i, char *buf) {
	thousands(i, buf);
}

static int failed = 0;

static void test(int i, const char *s) {
	char buf[500];
	towords(i, buf);
	if (0 != strcmp(s, buf)) {
		failed = 1;
		fprintf(stderr, "failed test for %d. Expected:\n\t%s\nactual:\n\t%.500s\n\n", i, s, buf);
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
	test(1234000890, "one billion two hundred and thirty-four million eight hundred and ninety");
	return failed;
}
