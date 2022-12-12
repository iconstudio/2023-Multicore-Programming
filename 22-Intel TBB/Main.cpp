#include "Main.hpp"

using namespace std;
using namespace std::chrono;
using namespace tbb;

atomic_int asum = 0;
volatile int vsum = 0;
constexpr size_t LoopTimes = 50000000;
string Data[100]{};
typedef concurrent_hash_map<string, int> StringTable;

struct Tally
{
	StringTable& table;
	Tally(StringTable& table_) : table(table_) {}

	void operator()(const blocked_range<string*> range) const
	{
		for (string* p = range.begin(); p != range.end(); ++p)
		{
			StringTable::accessor a;
			table.insert(a, *p);
			a->second += 1;
		}
	}
};

void CountOccurrences()
{
	// Construct empty table. 
	concurrent_unordered_map<string, int> table{};

	// Put occurrences into the table 
	parallel_for(size_t(0), size_t(100)
		, [&table](int i) {
		table[Data[i]]++;
	});

	// Display the occurrences 
	for (auto& t : table)
	{
		cout << t.first << " " << t.second << endl;
	}
}

int main()
{
	auto& target_adder = asum;

	cout << "Single Thread\n";
	auto start_clock = high_resolution_clock::now();

	for (size_t i = 0; i < LoopTimes; i++)
	{
		target_adder += 2;
	}

	auto now_clock = high_resolution_clock::now();

	auto period = now_clock - start_clock;
	auto ms = duration_cast<milliseconds>(period);

	cout << "Sum = " << target_adder << endl;
	cout << "Time = " << ms << endl;

	target_adder = 0;

	cout << "Multi Thread\n";
	start_clock = high_resolution_clock::now();

	parallel_for(size_t(0), LoopTimes, [&](int i) {
		target_adder += 2;
	});

	now_clock = high_resolution_clock::now();

	period = now_clock - start_clock;
	ms = duration_cast<milliseconds>(period);

	cout << "Sum = " << target_adder << endl;
	cout << "Time = " << ms << endl;
}
