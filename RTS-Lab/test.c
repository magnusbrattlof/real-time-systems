#include <stdlib.h>
#include <stdio.h>

int freqind[32] = {0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0};
int periods[25] = {2024, 1908, 1805, 1701, 1608, 1515, 1433, 1351, 1276, 1205, 1136, 1073, 1012, 956, 903, 852, 804, 759, 716, 676, 638, 602, 568, 536, 506};

void print_periods(int key);

void print_periods(int key) {
	int real_key = key;
	printf("For key: %d: \n", key);
	printf("Frequency index:\tPeriod:\n");
	int i, j;
	key += 10;
	for(i = 0; i <= 31; i++) {
		printf("%d\t%d\n",freqind[i] + real_key, periods[freqind[i] + key]);
	}
}

int main(void) {
	int key;
	printf("Enter key: ");
	scanf("%d", &key);
	print_periods(key);
}
