// 文件名：main.cpp
// 课程：数据结构实验 —— 实验二·队列应用：操作系统打印机管理器
// 功能：在前一版支持“小数秒/页 + 页数必须是正整数”的基础上，新增三文件持久化：
//       waiting.csv（等待中）、running.csv（进行中）、done.csv（已完成）
#include <bits/stdc++.h>
using namespace std;

struct PrintJob {
    int id = -1;
    string user;
    string doc;
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

static inline string csvEscape(const string& s) {
    bool need = false;
    for (char c : s) {
        if (c == '"' || c == ',' || c == '\n' || c == '\r') { need = true; break; }
    }
    if (!need) return s;
    string out = "\"";
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

    queue<PrintJob> waitQ;   // 等待队列（FIFO）
    vector<PrintJob> done;   // 完成日志

    bool busy = false;       // 打印机是否忙
    PrintJob current;        // 正在打印的任务
    int remainSec = 0;       // 当前任务剩余“整秒数”（向上取整）

    // —— 文件名（可按需修改）
    string fileWaiting = "waiting.csv";
    string fileRunning = "running.csv";
    string fileDone    = "done.csv";

    // —— 工具：格式化时间（把秒格式化为 mm:ss）
    static string fmt(int sec) {
        if (sec < 0) return "-";
        int m = sec / 60, s = sec % 60;
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
        return string(buf);
    }

    // ========== 持久化 ==========
    void saveWaiting() const {
        ofstream fout(fileWaiting, ios::trunc);
        // 表头：与结构体字段一一对应
        fout << "id,user,doc,pages,submitTime,startTime,finishTime\n";
        queue<PrintJob> tmp = waitQ;
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
        ofstream fout(fileRunning, ios::trunc);
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
        ofstream fout(fileDone, ios::trunc);
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
    int addJob(const string& user, const string& doc, int pages) {
        PrintJob j;
        j.id = nextId++;
        j.user = user;
        j.doc = doc;
        j.pages = pages;
        j.submitTime = currentTime;
        waitQ.push(j);
        saveWaiting(); // 等待队列变化
        return j.id;
    }

    // 取消等待中的任务（按 ID），返回是否找到并删除
    bool cancelJob(int id) {
        bool found = false;
        queue<PrintJob> q2;
        while (!waitQ.empty()) {
            auto x = waitQ.front(); waitQ.pop();
            if (x.id == id) {
                found = true; // 删除
            } else {
                q2.push(x);
            }
        }
        waitQ.swap(q2);
        saveWaiting(); // 等待队列变化
        return found;
    }

    // 设置速度：秒/页（支持小数，限定 > 0）
    void setSpeed(double sec_per_page) {
        if (sec_per_page <= 0) sec_per_page = 0.001; // 下限保护
        secPerPage = sec_per_page;
        // 速度变化不触发文件变化
    }

    // 每秒推进 dt（离散仿真主循环的一步）
    void tick(int dt = 1) {
        for (int step = 0; step < dt; ++step) {
            // 若空闲则尝试取新任务
            if (!busy) {
                if (!waitQ.empty()) {
                    current = waitQ.front(); waitQ.pop();
                    current.startTime = currentTime;
                    // 用向上取整把 “pages * (秒/页)” 转成整秒数
                    remainSec = (int)std::ceil(current.pages * secPerPage);
                    busy = true;
                    cout << "[开始] 任务#" << current.id
                         << " 用户:" << current.user
                         << " 文档:" << current.doc
                         << " 页数:" << current.pages
                         << " t=" << fmt(currentTime) << "\n";
                    // 队列与running变化
                    saveWaiting();
                    saveRunning();
                } else {
                    // 空闲：时钟仍然向前
                    currentTime++;
                    // running 文件保持空表（已在之前保存）
                    continue;
                }
            } else {
                // 正在忙：每秒更新一下 running（remainSec 会变化）
                // （不是必须，但写入一次可以让磁盘文件也“呼吸”）
                saveRunning();
            }

            // 若忙则执行1秒打印
            remainSec--;
            currentTime++;
            if (remainSec <= 0) {
                current.finishTime = currentTime;
                done.push_back(current);
                cout << "[完成] 任务#" << current.id
                     << " 等待:" << current.waitTime() << "秒"
                     << " 耗时:" << current.duration() << "秒"
                     << " 完成时刻 t=" << fmt(currentTime) << "\n";
                busy = false;
                // 任务从 running → done
                saveRunning(); // 此时会写出空表头
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

    // 查看等待队列（不破坏队列）
    void listWaiting() const {
        if (waitQ.empty()) {
            cout << "[等待队列] 空\n";
            return;
        }
        cout << "[等待队列]\n";
        queue<PrintJob> tmp = waitQ;
        while (!tmp.empty()) {
            const auto& j = tmp.front();
            cout << "  #"<< j.id
                 << "  用户:" << j.user
                 << "  文档:" << j.doc
                 << "  页数:" << j.pages
                 << "  提交t=" << fmt(j.submitTime)
                 << "\n";
            tmp.pop();
        }
    }

    // 查看已完成日志与统计
    void listDone() const {
        if (done.empty()) {
            cout << "[完成日志] 暂无\n";
            return;
        }
        long long sumWait = 0, sumDur = 0;
        cout << "[完成日志]\n";
        for (const auto& j : done) {
            cout << "  #"<< j.id
                 << "  用户:" << j.user
                 << "  文档:" << j.doc
                 << "  页数:" << j.pages
                 << "  提交:" << fmt(j.submitTime)
                 << "  开始:" << fmt(j.startTime)
                 << "  结束:" << fmt(j.finishTime)
                 << "  等待:" << j.waitTime() << "s"
                 << "  耗时:" << j.duration() << "s"
                 << "\n";
            sumWait += j.waitTime();
            sumDur  += j.duration();
        }
        cout << "== 统计: 完成任务数=" << done.size()
             << "  平均等待=" << (double)sumWait / done.size() << "s"
             << "  平均耗时=" << (double)sumDur  / done.size() << "s\n";
    }

    // 打印当前状态
    void status() const {
        ostringstream ss;
        ss.setf(std::ios::fixed); ss<<setprecision(3)<<secPerPage;

        cout << "[时钟] t=" << fmt(currentTime)
             << "  [速度] " << ss.str() << "秒/页  "
             << "[状态] " << (busy ? "打印中" : "空闲") << "\n";
        if (busy) {
            cout << "  进行中任务 #" << current.id
                 << " (" << current.user << "/" << current.doc << ") "
                 << "剩余约 " << remainSec << " 秒\n";
        }
    }
};

/*==================== 输入工具：严控“页数=正整数”，速度=正数（可小数） ====================*/

// 去除首尾空白
static inline string trim(const string& s) {
    size_t l = 0, r = s.size();
    while (l < r && isspace((unsigned char)s[l])) ++l;
    while (r > l && isspace((unsigned char)s[r-1])) --r;
    return s.substr(l, r - l);
}

// 判断是否为（可带正负号的）整数字符串（不含小数点/其他字符）
static inline bool isIntegerString(const string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[i] == '+' || s[i] == '-') ++i;
    if (i >= s.size()) return false;
    for (; i < s.size(); ++i) {
        if (!isdigit((unsigned char)s[i])) return false;
    }
    return true;
}

// 读取“严格正整数”（用于页数）
int readPositiveIntStrict(const string& prompt,
                          const string& err_msg = "请输入正整数（不要输入小数）：") {
    while (true) {
        cout << prompt << flush;
        string line; 
        if (!std::getline(cin, line)) return 0; // EOF 保护
        line = trim(line);
        if (isIntegerString(line)) {
            long long v = 0;
            try {
                v = stoll(line);
            } catch (...) {
                cout << err_msg << "\n";
                continue;
            }
            if (v > 0 && v <= INT_MAX) return (int)v;
        }
        cout << err_msg << "\n";
    }
}

// 读取“正数（可小数）”，用于速度
double readPositiveDouble(const string& prompt,
                          const string& err_msg = "请输入大于0的数字（可为小数）：") {
    while (true) {
        cout << prompt << flush;
        string line;
        if (!std::getline(cin, line)) return 1.0; // EOF 保护
        line = trim(line);
        if (line.empty()) { cout << err_msg << "\n"; continue; }
        try {
            size_t pos = 0;
            double v = stod(line, &pos);
            string rest = trim(line.substr(pos));
            if (!rest.empty() || !(v > 0)) {
                cout << err_msg << "\n"; 
                continue;
            }
            return v;
        } catch (...) {
            cout << err_msg << "\n";
        }
    }
}

// —— 保持原有：简单读整（用于菜单选择 / dt 等）
int readInt(const string& prompt) {
    cout << prompt<<flush;
    int x;
    while (!(cin >> x)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "请输入整数：";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 丢弃行尾
    return x;
}

string readLine(const string& prompt) {
    cout << prompt<<flush;
    string s;
    getline(cin, s);
    return s;
}

void menu() {
    cout << "\n===== 打印机管理器（队列 + 仿真）=====\n"
         << "1. 设置打印速度（秒/页）\n"
         << "2. 追加任务\n"
         << "3. 取消等待中的任务（按ID）\n"
         << "4. 查看等待队列\n"
         << "5. 查看已完成日志与统计\n"
         << "6. 模拟打印——按秒推进\n"
         << "7. 模拟打印——直接跑完当前队列\n"
         << "8. 随机生成若干任务（测试）\n"
         << "9. 查看系统状态\n"
         << "0. 退出\n"
         << "选择: "<< flush;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(&cout);

    PrintManager pm;
    // 程序启动时先把三文件写出“当前空状态”（可选）
    pm.saveAll();

    while (true) {
        menu();
        int op; 
        if (!(cin >> op)) break;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (op == 0) {
            cout << "再见！\n";
            break;
        } else if (op == 1) {
            double sp = readPositiveDouble("请输入速度（秒/页，>0，可小数）：");
            pm.setSpeed(sp);
            ostringstream ss;
            ss.setf(std::ios::fixed); ss<<setprecision(3)<<sp;
            cout << "已设置速度为 " << ss.str() << " 秒/页。\n";
        } else if (op == 2) {
            string user = readLine("用户：");
            string doc  = readLine("文档名：");
            int pages   = readPositiveIntStrict("页数（正整数）：",
                                                "页数需为正整数（不能是小数）。");
            if (pages <= 0) { cout << "页数需为正整数。\n"; continue; }
            int id = pm.addJob(user, doc, pages);
            cout << "已提交任务，ID=" << id << "，当前时刻t=" 
                 << PrintManager::fmt(pm.currentTime) << "\n";
        } else if (op == 3) {
            int id = readInt("输入要取消的任务ID：");
            bool ok = pm.cancelJob(id);
            cout << (ok ? "已取消该任务。\n" : "未在等待队列中找到该任务。\n");
        } else if (op == 4) {
            pm.listWaiting();
        } else if (op == 5) {
            pm.listDone();
        } else if (op == 6) {
            int dt = readInt("推进多少秒(dt)：");
            if (dt <= 0) { cout << "dt 需为正整数。\n"; continue; }
            pm.tick(dt);
        } else if (op == 7) {
            pm.runToEnd();
        } else if (op == 8) {
            int n = readInt("随机生成任务个数：");
            int minP = readInt("最小页数：");
            int maxP = readInt("最大页数：");
            if (n <= 0 || minP <= 0 || maxP < minP) {
                cout << "参数不合法。\n"; continue;
            }
            std::mt19937 rng((unsigned)time(nullptr));
            std::uniform_int_distribution<int> pages(minP, maxP);
            for (int i = 1; i <= n; ++i) {
                string user = "User" + to_string(i);
                string doc  = "Doc"  + to_string(i);
                int p = pages(rng);
                pm.addJob(user, doc, p); // 内部已保存 waiting.csv
            }
            cout << "已追加 " << n << " 个随机任务。\n";
        } else if (op == 9) {
            pm.status();
        } else {
            cout << "无效选择。\n";
        }
    }
    return 0;
}
