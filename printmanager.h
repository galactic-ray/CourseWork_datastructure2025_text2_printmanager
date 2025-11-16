#ifndef PRINTMANAGER_H
#define PRINTMANAGER_H

#include <string>
#include <queue>
#include <vector>
#include <fstream>
#include <cmath>
#include <sstream>
#include <iomanip>

struct PrintJob {
    int id = -1;
    std::string user;
    std::string doc;
    int pages = 0;
    int submitTime = 0; // 提交时刻（秒）
    int startTime = -1;
    int finishTime = -1;

    int waitTime() const {
        if (startTime < 0) return -1;
        return startTime - submitTime;
    }
    int duration() const {
        if (finishTime < 0 || startTime < 0) return -1;
        return finishTime - startTime;
    }
};

static inline std::string csvEscape(const std::string& s) {
    bool need = false;
    for (char c : s) {
        if (c == '"' || c == ',' || c == '\n' || c == '\r') { need = true; break; }
    }
    if (!need) return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\""; else out += c;
    }
    out += "\"";
    return out;
}

struct PrintManager {
    int currentTime = 0;      // 仿真时钟（秒）
    double secPerPage = 2.0;  // 速度：秒/页（支持小数）
    int nextId      = 1;

    std::queue<PrintJob> waitQ;   // 等待队列（FIFO）
    std::vector<PrintJob> done;   // 完成日志

    bool busy = false;       // 打印机是否忙
    PrintJob current;        // 正在打印的任务
    int remainSec = 0;       // 当前任务剩余"整秒数"（向上取整）

    // —— 文件名（可按需修改）
    std::string fileWaiting = "waiting.csv";
    std::string fileRunning = "running.csv";
    std::string fileDone    = "done.csv";

    // —— 工具：格式化时间（把秒格式化为 mm:ss）
    static std::string fmt(int sec) {
        if (sec < 0) return "-";
        int m = sec / 60, s = sec % 60;
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
        return std::string(buf);
    }

    // ========== 持久化 ==========
    void saveWaiting() const {
        std::ofstream fout(fileWaiting, std::ios::trunc);
        fout << "id,user,doc,pages,submitTime,startTime,finishTime\n";
        std::queue<PrintJob> tmp = waitQ;
        while (!tmp.empty()) {
            const auto& j = tmp.front();
            fout << j.id << ","
                 << csvEscape(j.user) << ","
                 << csvEscape(j.doc)  << ","
                 << j.pages << ","
                 << j.submitTime << ","
                 << j.startTime  << ","
                 << j.finishTime << "\n";
            tmp.pop();
        }
    }

    void saveRunning() const {
        std::ofstream fout(fileRunning, std::ios::trunc);
        fout << "id,user,doc,pages,submitTime,startTime,finishTime,remainSec\n";
        if (busy) {
            fout << current.id << ","
                 << csvEscape(current.user) << ","
                 << csvEscape(current.doc)  << ","
                 << current.pages << ","
                 << current.submitTime << ","
                 << current.startTime  << ","
                 << current.finishTime << ","
                 << remainSec << "\n";
        }
    }

    void saveDone() const {
        std::ofstream fout(fileDone, std::ios::trunc);
        fout << "id,user,doc,pages,submitTime,startTime,finishTime\n";
        for (const auto& j : done) {
            fout << j.id << ","
                 << csvEscape(j.user) << ","
                 << csvEscape(j.doc)  << ","
                 << j.pages << ","
                 << j.submitTime << ","
                 << j.startTime  << ","
                 << j.finishTime << "\n";
        }
    }

    void saveAll() const {
        saveWaiting();
        saveRunning();
        saveDone();
    }

    // 追加任务：入队
    int addJob(const std::string& user, const std::string& doc, int pages) {
        PrintJob j;
        j.id = nextId++;
        j.user = user;
        j.doc = doc;
        j.pages = pages;
        j.submitTime = currentTime;
        waitQ.push(j);
        saveWaiting();
        return j.id;
    }

    // 取消等待中的任务（按 ID），返回是否找到并删除
    bool cancelJob(int id) {
        bool found = false;
        std::queue<PrintJob> q2;
        while (!waitQ.empty()) {
            auto x = waitQ.front(); waitQ.pop();
            if (x.id == id) {
                found = true;
            } else {
                q2.push(x);
            }
        }
        waitQ.swap(q2);
        saveWaiting();
        return found;
    }

    // 设置速度：秒/页（支持小数，限定 > 0）
    void setSpeed(double sec_per_page) {
        if (sec_per_page <= 0) sec_per_page = 0.001;
        secPerPage = sec_per_page;
    }

    // 每秒推进 dt（离散仿真主循环的一步）
    void tick(int dt = 1) {
        for (int step = 0; step < dt; ++step) {
            if (!busy) {
                if (!waitQ.empty()) {
                    current = waitQ.front(); waitQ.pop();
                    current.startTime = currentTime;
                    remainSec = (int)std::ceil(current.pages * secPerPage);
                    busy = true;
                    saveWaiting();
                    saveRunning();
                } else {
                    currentTime++;
                    continue;
                }
            } else {
                saveRunning();
            }

            remainSec--;
            currentTime++;
            if (remainSec <= 0) {
                current.finishTime = currentTime;
                done.push_back(current);
                busy = false;
                saveRunning();
                saveDone();
            }
        }
    }

    // 一直跑到队列清空且当前任务完成
    void runToEnd() {
        while (busy || !waitQ.empty()) {
            tick(1);
        }
    }

    // 获取等待队列的副本（用于显示）
    std::vector<PrintJob> getWaitingJobs() const {
        std::vector<PrintJob> result;
        std::queue<PrintJob> tmp = waitQ;
        while (!tmp.empty()) {
            result.push_back(tmp.front());
            tmp.pop();
        }
        return result;
    }

    // 获取统计信息
    struct Statistics {
        int totalCompleted = 0;
        double avgWaitTime = 0.0;
        double avgDuration = 0.0;
    };

    Statistics getStatistics() const {
        Statistics stats;
        stats.totalCompleted = done.size();
        if (done.empty()) return stats;
        
        long long sumWait = 0, sumDur = 0;
        for (const auto& j : done) {
            sumWait += j.waitTime();
            sumDur  += j.duration();
        }
        stats.avgWaitTime = (double)sumWait / done.size();
        stats.avgDuration = (double)sumDur  / done.size();
        return stats;
    }
};

#endif // PRINTMANAGER_H

