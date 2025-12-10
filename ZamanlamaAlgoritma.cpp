
// Teknik olarak çoğu kısım yapay zekadan yardım alınarak veya yapay zeka ile düzeltmeler yaparak yapılmıştır. 
// Elimden geldiğince kendi mantığımla kodları ve algoritmaları yazmaya çalıştım.
// Mehmet Nazım Coşkun
// 20252013016


#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <thread>
#include <mutex>
#include <iomanip>
#include <sstream>
#include <cmath>

using namespace std;

struct Process {
    string pid;
    int arrival;
    int burst;
    int priority;
    int remaining;
    int start_time;
    int finish_time;
    int waiting_time;
    int turnaround_time;
};

class Scheduler {
private:
    vector<Process> processes;
    vector<pair<int, pair<string, int>>> timeline; // time, pid, duration
    string algorithm_name;

public:
    Scheduler(const vector<Process>& procs, string name)
        : processes(procs), algorithm_name(name) {
        for (auto& p : processes) {
            p.remaining = p.burst;
        }
    }

    void FCFS() {
        timeline.clear();
        vector<Process> sorted = processes;
        sort(sorted.begin(), sorted.end(),
            [](const Process& a, const Process& b) {
                return a.arrival < b.arrival;
            });

        int current_time = 0;
        for (auto& p : sorted) {
            if (current_time < p.arrival) {
                timeline.push_back({ current_time, {"IDLE", p.arrival - current_time} });
                current_time = p.arrival;
            }
            timeline.push_back({ current_time, {p.pid, p.burst} });
            p.start_time = current_time;
            current_time += p.burst;
            p.finish_time = current_time;
        }
    }

    void PreemptiveSJF() {
        timeline.clear();
        vector<Process> procs = processes;
        int current_time = 0;
        int completed = 0;
        int n = procs.size();

        while (completed < n) {
            int idx = -1;
            int shortest = INT_MAX;

            for (int i = 0; i < n; i++) {
                if (procs[i].arrival <= current_time && procs[i].remaining > 0) {
                    if (procs[i].remaining < shortest) {
                        shortest = procs[i].remaining;
                        idx = i;
                    }
                }
            }

            if (idx == -1) {
                timeline.push_back({ current_time, {"IDLE", 1} });
                current_time++;
                continue;
            }

            int duration = 1;
            timeline.push_back({ current_time, {procs[idx].pid, duration} });
            procs[idx].remaining -= duration;

            if (procs[idx].remaining == 0) {
                completed++;
                procs[idx].finish_time = current_time + duration;
            }
            current_time += duration;
        }
    }

    void NonPreemptiveSJF() {
        timeline.clear();
        vector<Process> procs = processes;
        sort(procs.begin(), procs.end(),
            [](const Process& a, const Process& b) {
                return a.arrival < b.arrival;
            });

        int current_time = 0;
        int completed = 0;
        int n = procs.size();
        vector<bool> executed(n, false);

        while (completed < n) {
            int idx = -1;
            int shortest = INT_MAX;

            for (int i = 0; i < n; i++) {
                if (!executed[i] && procs[i].arrival <= current_time) {
                    if (procs[i].burst < shortest) {
                        shortest = procs[i].burst;
                        idx = i;
                    }
                }
            }

            if (idx == -1) {
                timeline.push_back({ current_time, {"IDLE", 1} });
                current_time++;
                continue;
            }

            timeline.push_back({ current_time, {procs[idx].pid, procs[idx].burst} });
            procs[idx].start_time = current_time;
            current_time += procs[idx].burst;
            procs[idx].finish_time = current_time;
            executed[idx] = true;
            completed++;
        }
    }

    void RoundRobin(int quantum = 2) {
        timeline.clear();
        vector<Process> procs = processes;
        queue<int> ready_queue;
        int current_time = 0;
        int n = procs.size();
        vector<int> arrival_copy(n);

        for (int i = 0; i < n; i++) {
            arrival_copy[i] = procs[i].arrival;
        }

        // İlk gelen process'leri ekle
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival <= current_time) {
                ready_queue.push(i);
            }
        }

        while (true) {
            bool done = true;
            for (int i = 0; i < n; i++) {
                if (procs[i].remaining > 0) {
                    done = false;
                    break;
                }
            }
            if (done) break;

            if (ready_queue.empty()) {
                timeline.push_back({ current_time, {"IDLE", 1} });
                current_time++;

                // Yeni process'leri kontrol et
                for (int i = 0; i < n; i++) {
                    if (procs[i].arrival <= current_time && procs[i].remaining > 0) {
                        bool in_queue = false;
                        queue<int> temp = ready_queue;
                        while (!temp.empty()) {
                            if (temp.front() == i) {
                                in_queue = true;
                                break;
                            }
                            temp.pop();
                        }
                        if (!in_queue) ready_queue.push(i);
                    }
                }
                continue;
            }

            int idx = ready_queue.front();
            ready_queue.pop();

            int exec_time = min(quantum, procs[idx].remaining);
            timeline.push_back({ current_time, {procs[idx].pid, exec_time} });

            procs[idx].remaining -= exec_time;
            current_time += exec_time;

            // Yeni process'leri ekle
            for (int i = 0; i < n; i++) {
                if (procs[i].arrival <= current_time && procs[i].remaining > 0 && i != idx) {
                    bool in_queue = false;
                    queue<int> temp = ready_queue;
                    while (!temp.empty()) {
                        if (temp.front() == i) {
                            in_queue = true;
                            break;
                        }
                        temp.pop();
                    }
                    if (!in_queue) ready_queue.push(i);
                }
            }

            if (procs[idx].remaining > 0) {
                ready_queue.push(idx);
            }
            else {
                procs[idx].finish_time = current_time;
            }
        }
    }

    void calculateMetrics() {
        for (auto& p : processes) {
            p.turnaround_time = p.finish_time - p.arrival;
            p.waiting_time = p.turnaround_time - p.burst;
        }
    }

    void saveResults() {
        string filename = algorithm_name + "_results.txt";
        ofstream file(filename);

        file << "Zaman Tablosu:\n";
        for (auto& event : timeline) {
            file << "[ " << setw(3) << event.first << " ] - - "
                << setw(4) << event.second.first << " - - [ "
                << setw(5) << event.second.second << " ]\n";
        }

        calculateMetrics();

        int total_waiting = 0;
        int total_turnaround = 0;
        int max_waiting = 0;
        int max_turnaround = 0;

        file << "\nProcess Bilgileri:\n";
        file << setw(8) << "PID" << setw(10) << "Arrival" << setw(10) << "Burst"
            << setw(10) << "Finish" << setw(10) << "Waiting" << setw(15) << "Turnaround\n";

        for (auto& p : processes) {
            file << setw(8) << p.pid << setw(10) << p.arrival << setw(10) << p.burst
                << setw(10) << p.finish_time << setw(10) << p.waiting_time
                << setw(15) << p.turnaround_time << "\n";

            total_waiting += p.waiting_time;
            total_turnaround += p.turnaround_time;
            max_waiting = max(max_waiting, p.waiting_time);
            max_turnaround = max(max_turnaround, p.turnaround_time);
        }

        file << "\nMaksimum Bekleme Süresi: " << max_waiting << "\n";
        file << "Ortalama Bekleme Süresi: " << (double)total_waiting / processes.size() << "\n";
        file << "Maksimum Tamamlanma Süresi: " << max_turnaround << "\n";
        file << "Ortalama Tamamlanma Süresi: " << (double)total_turnaround / processes.size() << "\n";

        file.close();
    }
};

vector<Process> readCSV(const string& filename) {
    vector<Process> processes;
    ifstream file(filename);
    string line;

    getline(file, line); // header

    while (getline(file, line)) {
        stringstream ss(line);
        string pid, arrival, burst, priority;

        getline(ss, pid, ',');
        getline(ss, arrival, ',');
        getline(ss, burst, ',');
        getline(ss, priority, ',');

        Process p;
        p.pid = pid;
        p.arrival = stoi(arrival);
        p.burst = stoi(burst);
        p.priority = stoi(priority);

        processes.push_back(p);
    }

    return processes;
}

void runAlgorithm(const vector<Process>& processes, const string& algo_name, int type) {
    Scheduler scheduler(processes, algo_name);

    switch (type) {
    case 1: scheduler.FCFS(); break;
    case 2: scheduler.PreemptiveSJF(); break;
    case 3: scheduler.NonPreemptiveSJF(); break;
    case 4: scheduler.RoundRobin(); break;
        // Priority scheduling'leri de ekleyebilirsiniz
    }

    scheduler.saveResults();
    cout << algo_name << " tamamlandı" << endl;
}

int main() {
    // CSV dosyalarını oku
    auto case1 = readCSV("case1.csv");
    auto case2 = readCSV("case2.csv");

    
    vector<thread> threads;

    // sadece case 1 
    threads.push_back(thread(runAlgorithm, case1, "FCFS_case1", 1));
    threads.push_back(thread(runAlgorithm, case1, "PreemptiveSJF_case1", 2));
    threads.push_back(thread(runAlgorithm, case1, "NonPreemptiveSJF_case1", 3));
    threads.push_back(thread(runAlgorithm, case1, "RoundRobin_case1", 4));

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
