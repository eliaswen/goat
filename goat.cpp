#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <atomic>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>

using namespace std;

// Format numbers with spaces for readability
string format_number(long long num) {
    stringstream ss;
    ss.imbue(locale("en_US.utf8"));
    ss << fixed << num;
    string result = ss.str();
    for (size_t i = result.find('.') - 3; i > 0 && isdigit(result[i - 1]); i -= 3) {
        result.insert(i, " ");
    }
    return result;
}

void run_simulation(long long iterations, atomic<long long>& a_wins, atomic<long long>& b_wins) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(0, 2);

    for (long long i = 0; i < iterations; ++i) {
        int goat = dist(gen);  // Goat's position
        int choice = dist(gen); // Person A and B's initial choice

        // Presenter opens a door that's not the goat and not the chosen door
        int open_door;
        do {
            open_door = dist(gen);
        } while (open_door == goat || open_door == choice);

        // Person A stays, Person B switches
        if (choice == goat) {
            a_wins++;
        }
        if (3 - choice - open_door == goat) { // Remaining door
            b_wins++;
        }
    }
}

int main(int argc, char* argv[]) {
    long long iterations = 0;
    long long threads_count = 0;
    bool skip_confirmation = false;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0) {
            if (i + 1 < argc) {
                threads_count = stoll(argv[++i]);
            } else {
                cerr << "Error: Missing value for threads." << endl;
                return 1;
            }
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--iterations") == 0) {
            if (i + 1 < argc) {
                iterations = stoll(argv[++i]);
            } else {
                cerr << "Error: Missing value for iterations." << endl;
                return 1;
            }
        } else if (strcmp(argv[i], "-im") == 0 || strcmp(argv[i], "--iterations-millions") == 0) {
            if (i + 1 < argc) {
                iterations = stoll(argv[++i]) * 1'000'000;
            } else {
                cerr << "Error: Missing value for iterations in millions." << endl;
                return 1;
            }
        } else if (strcmp(argv[i], "-ib") == 0 || strcmp(argv[i], "--iterations-billions") == 0) {
            if (i + 1 < argc) {
                iterations = stoll(argv[++i]) * 1'000'000'000;
            } else {
                cerr << "Error: Missing value for iterations in billions." << endl;
                return 1;
            }
        } else if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yes") == 0) {
            skip_confirmation = true;
        } else {
            cerr << "Error: Unknown argument: " << argv[i] << endl;
            return 1;
        }
    }

    // Ask for input if not provided through command-line arguments
    if (iterations == 0) {
        cout << "Enter the number of iterations to run: ";
        while (!(cin >> iterations) || iterations < 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a non-negative integer: ";
        }
    }

    if (threads_count == 0) {
        cout << "Enter the number of threads to run: ";
        while (!(cin >> threads_count) || threads_count <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a positive integer: ";
        }
    }

    if (!skip_confirmation) {
        cout << "Press Enter to confirm and start...";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cin.get();
    }

    atomic<long long> a_wins(0), b_wins(0);
    vector<thread> threads;

    auto start_time = chrono::high_resolution_clock::now();
    
    // Divide iterations among threads
    long long iterations_per_thread = iterations / threads_count;
    long long remainder = iterations % threads_count;

    for (long long i = 0; i < threads_count; ++i) {
        long long this_thread_iterations = iterations_per_thread + (i < remainder ? 1 : 0);
        threads.emplace_back(run_simulation, this_thread_iterations, ref(a_wins), ref(b_wins));
    }

    // Monitor progress
    thread monitor([&]() {
        long long last_total_done = 0;
        auto last_time = chrono::high_resolution_clock::now();

        while (true) {
            auto now = chrono::high_resolution_clock::now();
            auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - start_time).count();
            auto time_since_last = chrono::duration_cast<chrono::milliseconds>(now - last_time).count();
            long long total_done = a_wins + b_wins;
            
            if (total_done >= iterations) break;

            double percent_done = 100.0 * total_done / iterations;
            double time_per_million = (total_done > last_total_done && time_since_last > 0) 
                                      ? (1000000.0 * time_since_last) / (total_done - last_total_done) 
                                      : 0;

            cout << "\rProgress: " << format_number(total_done) << " / " << format_number(iterations)
                 << " (" << fixed << setprecision(2) << percent_done << "%) ";
            cout << "| A Wins: " << format_number(a_wins) << " ("
                 << setprecision(2) << 100.0 * a_wins / total_done << "%) ";
            cout << "| B Wins: " << format_number(b_wins) << " ("
                 << setprecision(2) << 100.0 * b_wins / total_done << "%) ";
            cout << "| Time per million: " << fixed << setprecision(2) << time_per_million << " ms      ";
            cout.flush();

            last_total_done = total_done;
            last_time = now;

            this_thread::sleep_for(chrono::milliseconds(25)); // Add delay
        }
    });

    // Wait for threads to finish
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    // Wait for monitor to finish
    if (monitor.joinable()) monitor.join();

    auto end_time = chrono::high_resolution_clock::now();
    auto total_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

    cout << "\n\nSimulation complete!\n";
    cout << "Total iterations: " << format_number(iterations) << "\n";
    cout << "Person A Wins: " << format_number(a_wins) << " ("
         << setprecision(2) << 100.0 * a_wins / iterations << "%)\n";
    cout << "Person B Wins: " << format_number(b_wins) << " ("
         << setprecision(2) << 100.0 * b_wins / iterations << "%)\n";
    cout << "Total time: " << total_time << " ms\n";
    cout << "Average time per million iterations: " << fixed << setprecision(2)
         << (1000000.0 * total_time / iterations) << " ms\n";

    return 0;
}