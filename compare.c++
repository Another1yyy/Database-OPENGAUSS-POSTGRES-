#include <iostream>
#include <vector>
#include <string>
#include <functional> // 用于 std::function，定义查询条件
#include <algorithm>  // 用于 std::copy_if, std::find_if
#include<any>
#include<filesystem>
#include<fstream>
#include<sstream>
#include<functional>
#include<chrono>  
using namespace std;
namespace fs = std::filesystem;

// 定义行和列
using Row = vector<string>;
using Col = vector<string>;
const string path = "C:\\Coding\\DATABASE\\data\\yellow.csv";
const string name_default = "movieid";
//name is the 表头
struct Header {
    vector<string> names;
};

class Timer {
private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
    long long m_elapsed = 0;

    bool m_stopped = false;
public:
    explicit Timer(const std::string& name = "Operation")
        : m_name(name), m_start(chrono::high_resolution_clock::now()) {

    }

    ~Timer() {
        stop();
    }
    long long stop() {
        if (m_stopped) return m_elapsed;

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start);
        m_elapsed = duration.count();
        m_stopped = true;

        std::cout << m_name << "took: " << m_elapsed << "ms/n";
        return m_elapsed;

    }

    long long elapsed() const {
        if (m_stopped) return m_elapsed;
        auto current = chrono::high_resolution_clock::now();
        return chrono::duration_cast<std::chrono::milliseconds>(current - m_start).count();
    }
    // 获取当前已耗时（秒），不停止计时
    double elapsedSeconds() const {
        return elapsed() / 1000.0;
    }

};

bool key_string_match(const string key, const string str) {
    return str.find(key) != string::npos;
}

class table {
private:
    Header header;
    vector<Row> datas;

public:
    void insert(Row r) {
        //check if the row is valid and then insert
        for (string& a : r) {
            if (a.empty()) {
                a = "NULL";
            }
       }
        datas.push_back(r);
        return;

    }


    bool change(string colname, string keyword,string changed) {
        int col_id = -1;
        for (int i = 0; i < header.names.size(); i++) {
            if (header.names[i] == colname) {
                col_id = i;
            }
        }
        if (col_id == -1) {
            std::cout << "Error:coresponding colname not found!";
                return false;

        }
        for (int i = 0; i < datas.size(); i++) {
            //should check contain but not equal
            if (key_string_match(keyword, datas[i][col_id])) {
                datas[i][col_id] = changed;
            }
        }
    }
    vector<Row> search_by_upper_bound(double max_value, const string& colname = "trip_distance") {
        int col_id = -1;
        vector<int> index;
        Timer bound_timer("upper_bound_search");

        // 查找列索引
        for (int i = 0; i < header.names.size(); i++) {
            if (header.names[i] == colname) {
                col_id = i;
                break;
            }
        }

        if (col_id == -1) {
            cerr << "Error: Column '" << colname << "' not found!" << endl;
            return {};
        }

        vector<Row> result;

        // 遍历数据，查找小于max_value的行
        for (int i = 0; i < datas.size(); i++) {
            try {
                // 将字符串转换为double进行比较
                double value = std::stod(datas[i][col_id]);
                if (value < max_value) {
                    index.push_back(i);
                }
            }
            catch (const std::invalid_argument& e) {
                // 处理非数值数据（如"NULL"或无效格式）
                cerr << "Warning: Invalid numeric value at row " << i
                    << ", column '" << colname << "': " << datas[i][col_id] << endl;
            }
            catch (const std::out_of_range& e) {
                // 处理数值超出范围的情况
                cerr << "Warning: Numeric value out of range at row " << i
                    << ": " << datas[i][col_id] << endl;
            }
        }

        long long search_time = bound_timer.stop();
        std::cout << "Upper bound search time: " << search_time << "ms, found "
            << index.size() << " rows" << endl;

        // 收集结果
        result.reserve(index.size());
        for (size_t idx : index) {
            result.push_back(datas[idx]);
        }

        return result;
    }
    vector<Row> search(const string &keyword, const string &colname) {
        int col_id = 0;
        vector<int> index;
        Timer pure_search_timer("pure_search_bykey");
        vector<Row> result;
        for (int i = 0; i < header.names.size(); i++) {
            if (header.names[i] == colname) {
                col_id = i;
            }
        }
       
        vector<string> column_data;
        column_data.reserve(datas.size());
        for (const auto& row : datas) {
            column_data.push_back(row[col_id]);
        }
        for (int i = 0; i < column_data.size(); i++) {
            //should check contain but not equal
            if (key_string_match(keyword, datas[i][col_id])) {
                index.push_back(i);
            }
        }
        long long pure_search_time = pure_search_timer.stop();
        std::cout << "pure searching time" << pure_search_time << endl;
        for (size_t idx : index) {
            result.push_back(datas[idx]);
        }

        return result;


    }
    vector<Row> search_equal(const string &keyword,const string &colname) {
        int col_id = 0;
        vector<int> index;
        Timer pure_equal_timer("pure_euqul_searchtime:");
        for (int i = 0; i < header.names.size(); i++) {
            if (header.names[i] == colname) {
                col_id = i;
            }
        }

        vector<Row> result;
        vector<string> column_data;
        column_data.reserve(datas.size());
        for (const auto& row : datas) {
            column_data.push_back(row[col_id]);
        }
        for (int i = 0; i < column_data.size(); i++) {
            //should check contain but not equal
            if (keyword == column_data[i]) {
                index.push_back(i);
            }
        }
        long long pure_equal_time = pure_equal_timer.stop();
        std::cout << "pure_equal_time" << pure_equal_time << endl;
        for (size_t idx : index) {
            result.push_back(datas[idx]);
        }   
        return result;

    }
    void rewrite(const string& key, const string &changeto,const string &colname) {
        int col_id = 0;
        vector<int> index;
        Timer write_timer("write timer start");
        for (int i = 0; i < header.names.size(); i++) {
            if (header.names[i] == colname) {
                col_id = i;
            }
        }
      
        for (int i = 0; i < datas.size(); i++) {
            //should check contain but not equal
            if (key == datas[i][col_id]) {
                index.push_back(i);
            }
        }
        Timer pure_write_timer("pure writing starts");
        for (size_t idx : index) {
            datas[idx][col_id] = changeto;
        }
        long long pure_write_time = pure_write_timer.stop();
        long long write_time = write_timer.stop();
        std::cout << "pure writing time : " << pure_write_time;
        std::cout << "write total time:" << write_time << endl;

        
        
     
    }
    vector<Row> search_byrange(const string& keyword, const string& colname, const string) {
        
    }
    table load_csv(string path) {
        table result;
        fs::path p(path);
        fs::path abs_path = fs::absolute(p); // 转成绝对路径
        if (!fs::exists(abs_path)) {
            cerr << "ERROR: File does not exist: " << abs_path << endl;
            return result;
        }

        
        ifstream file(abs_path);
        if (!file.is_open()) {
            cerr << " ERROR : cannot open file";
        }
        string line;
        if (getline(file, line)) {
            header.names = parse_line(line);

        }
        else {
            cerr << "ERROR: CSV file is empty or could not read header." << endl;
            return result; // 返回空表
        }
        while (getline(file, line)) {
            if (line.empty()) continue;
            Row row = parse_line(line);
            datas.push_back(row);
        }
        return result;

    }

    vector<string> parse_line(const string& line) {
        vector<string> result;
        string cell;
        bool quoted = false;
        for (char c : line) {
            if (c == '"') {
                quoted = !quoted;
            }
            else if (c == ',' && !quoted) {
                result.push_back(cell);
                cell.clear();
            }
            else {
                cell += c;

            }
        }
        result.push_back(cell);
        return result;

    }

};

int main() {
  /*  Timer t("读取csv");*/
    Timer main_timer("main_timer");
    table tab;

    bool equal_search = false;
    tab.load_csv(path);
   
    int times = 5;
    while (times > 0) {
       
        string name;
        string key;
        string operation;
        cout << "enter the operation" << endl;
        cin >> operation;
        cout << "enter the search target" << endl;
        cin >> name;
        cout << "enter the keyword" << endl;
        cin >> key;
        Timer function_timer("running");
        if (operation == "equal") {
            vector<Row> res = tab.search_equal(key, name);
        }

        else if (operation == "search") {
            vector<Row> res = tab.search(key, name);
        }
        else if (operation == "write") {
            std::cout << "enter the changeto target" << endl;
            string changeto;
            cin >> changeto;
            tab.rewrite(key, changeto, name);

        }
        else if (operation == "bound") {
            double threshold;
            cout << "Enter the upper bound value (e.g., 1.5): ";
            cin >> threshold;

            if (cin.fail()) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cerr << "Error: Please enter a valid number!" << endl;
            }
            else {
                vector<Row> res = tab.search_by_upper_bound(threshold, name);
                cout << "Found " << res.size() << " rows with " << name
                    << " < " << threshold << endl;

                // 可选：显示前几条结果
                int show_count = min(5, (int)res.size());
                for (int i = 0; i < show_count; i++) {
                    cout << "Row " << i + 1 << ": ";
                    for (const auto& cell : res[i]) {
                        cout << cell << " ";
                    }
                    cout << endl;
                }
            }
        }
        long long time = function_timer.stop();
        std::cout << "timer stop" << endl;
        /*for (int i = 0; i < res.size(); i++) {
            for (int j = 0; j < res[i].size(); j++) {
                cout << res[i][j] << " ";
            }
            cout << endl;
        }   */
        main_timer.stop();
        std::cout << "function_time:" << " " << time << endl;
        times--;
    }
  /*  cin.ignore();
  * }
    cin.get();*/
}

