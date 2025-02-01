#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>

using namespace std;

class VendingMachine {
private:
    sqlite3* db;
    char* err_msg = nullptr;
    int rc;
    int max_collection = 100;
    const vector<int> bank_note = {100, 20, 10, 5, 1};

    void CreateDatabase() { // create or open an existing database; then make a table for stocks and payment
        // create / open a database
        rc = sqlite3_open("VendingMachineDatabase.db", &db);
        if (rc) cerr << "Cannot open database" << endl;

        // create stock table
        const char* stock_table = R"(
            CREATE TABLE IF NOT EXISTS stocks_67011140 (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                price TEXT NOT NULL,
                amount INTEGER
            );
        )";
        rc = sqlite3_exec(db, stock_table, nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK) {
            cerr << "SQL error: " << err_msg << endl; 
            sqlite3_free(err_msg);
        }

        // create change box table, this table will only use one element
        const char* change_box_table = R"(
            CREATE TABLE IF NOT EXISTS change_box (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                thb_100 INTEGER,
                thb_20 INTEGER,
                thb_10 INTEGER,
                thb_5 INTEGER,
                thb_1 INTEGER
            ); 
        )"; 
        rc = sqlite3_exec(db, change_box_table, nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK) {
            cerr << "SQL error: " << err_msg << endl; 
            sqlite3_free(err_msg);
        }

        // create collection box table, this table will only use one element
        const char* collection_box_table = R"(
            CREATE TABLE IF NOT EXISTS collection_box (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                thb_100 INTEGER,
                thb_20 INTEGER,
                thb_10 INTEGER,
                thb_5 INTEGER,
                thb_1 INTEGER
            );
        )";
        rc = sqlite3_exec(db, collection_box_table, nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK) {
            cerr << "SQL error: " << err_msg << endl; 
            sqlite3_free(err_msg);
        }
    }
    int GetRowCount(const string& table_name) { // return the amount of row in the table, 0 if nothing in table
        string query = "SELECT COUNT(*) FROM " + table_name + ";";
        int row_count = 0;

        // callback function
        auto RowCountCallback = [](void* data, int argc, char** argv, char** az_col_name) -> int {
            int* row_count_ptr = static_cast<int*>(data);
            if (argc > 0 && argv[0]) *row_count_ptr = stoi(argv[0]);
            return 0;
        };
        rc = sqlite3_exec(db, query.c_str(), RowCountCallback, &row_count, &err_msg);
        if (rc != SQLITE_OK) {
            cerr << "SQL error: " << err_msg << endl; 
            sqlite3_free(err_msg);
        }

        return row_count;
    }
    vector<string> GetRowValue(const string table_name, const string condition) {
        vector<string> row_value;
        string query = "SELECT * FROM " + table_name + " WHERE " + condition + ";";

        auto callback = [](void* data, int argc, char** argv, char** az_col_name) -> int {
            auto* row_value_ptr = static_cast<vector<string>*>(data);
            for (int i = 0; i < argc; i++) {
                row_value_ptr->push_back(argv[i] ? argv[i] : "NULL");
            }
            return 0;
        };
        rc = sqlite3_exec(db, query.c_str(), callback, &row_value, &err_msg);
        if (rc != SQLITE_OK) {
            cerr << "SQL error: " << err_msg << endl; 
            sqlite3_free(err_msg);
        }

        return row_value;
    }
    vector<string> GetColumnValue(const string table_name, const string column_name) {
        vector<string> column_value;
        string query = "SELECT " + column_name + " FROM " + table_name + ";";

        auto callback = [](void* data, int argc, char** argv, char** az_col_name) -> int {
            auto* col_value_ptr = static_cast<vector<string>*>(data);
            if (argc > 0 && argv[0]) {
                col_value_ptr -> push_back(argv[0]);
            }
            return 0;
        };
        rc = sqlite3_exec(db, query.c_str(), callback, &column_value, &err_msg);
        if (rc != SQLITE_OK) {
            cerr << "SQL error: " << err_msg << endl; 
            sqlite3_free(err_msg);
        }

        return column_value;
    }
    void PrintTable(const string table_name) {
        const int total_row = GetRowCount(table_name);
        cout << "--------------------------------------------------------------------------------------------------------------------------" << endl;

        if (table_name == "stocks_67011140") {
            cout << left << setw(10) << "ID" << setw(30) << "Name" << setw(20) << "Price" << "Amount" << endl;
            cout << "--------------------------------------------------------------------------------------------------------------------------" << endl;
            for (int i = 1; i <= total_row; i++) {
                vector<string> values = GetRowValue(table_name, "id = " + to_string(i));
                cout << left << setw(10) << values[0] << setw(30) << values[1] << setw(20) << values[2] << values[3] << endl;
            }
        } 
        if (table_name == "change_box" || table_name == "collection_box") {
            cout << left << setw(20) << "ID" << setw(20) << "100-THB" << setw(20) << "20-THB" << setw(20) << "10-THB" << setw(20) << "5-THB" << "1-THB" << endl;
            cout << "--------------------------------------------------------------------------------------------------------------------------" << endl;
            for (int i = 1; i <= total_row; i++) {
                vector<string> values = GetRowValue(table_name, "id = " + to_string(i));
                cout << left << setw(20) << values[0] << setw(20) << values[1] << setw(20) << values[2] << setw(20) << values[3] << setw(20) << values[4] << values[5] << endl;
            }
        }
    }
    void SetChangeBox(int b100, int b20, int b10, int b5, int b1) {
        string query;
        if (GetRowCount("change_box") <= 0) {
            query = "INSERT INTO change_box (thb_100, thb_20, thb_10, thb_5, thb_1) VALUES (" + 
                    to_string(b100) + ", " + to_string(b20) + ", " + to_string(b10) + ", " + to_string(b5) + ", " + to_string(b1) + ");";
        } else { // get the value of each detomination and increment it 
            vector<int> denomination_value; // thb_100, thb_20, thb_10, thb_5, thb_1
            vector<string> row_value = GetRowValue("change_box", "id = 1");

            // get row_value's value into denomination_value 
            for (int i = 1; i < row_value.size(); i++) {
                denomination_value.push_back(stoi(row_value[i]));
            }

            // set the query that will increment depend on returned value
            query = "UPDATE change_box SET thb_100 = " + to_string(b100 + denomination_value[0]) +
                    ", thb_20 = " + to_string(b20 + denomination_value[1]) +
                    ", thb_10 = " + to_string(b10 + denomination_value[2]) + 
                    ", thb_5 = " + to_string(b5 + denomination_value[3]) + 
                    ", thb_1 = " + to_string(b1 + denomination_value[4]) + 
                    " WHERE id = 1;";
        }
        rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK) {
            cerr << "SQL error: " << err_msg << endl; 
            sqlite3_free(err_msg);
        }
    }
    void SetCollectionBox() { // this method will only use once in contructor, since it just collecting data
        if (GetRowCount("collection_box") <= 0) {
            const char* query = "INSERT INTO collection_box (thb_100, thb_20, thb_10, thb_5, thb_1) VALUES (0, 0, 0, 0, 0);";
            rc = sqlite3_exec(db, query, nullptr, nullptr, &err_msg);
            if (rc != SQLITE_OK) {
                cerr << "SQL error: " << err_msg << endl; 
                sqlite3_free(err_msg);
            }
        }
    }
    int EmptyCollection() { // this method will reset the collection box and return the amount of money that the admin received
        vector<int> denomination_value; // thb_100, thb_20, thb_10, thb_5, thb_1
        vector<string> row_value = GetRowValue("collection_box", "id = 1");
        int sum;

        for (int i = 1; i < row_value.size(); i++) {
            denomination_value.push_back(stoi(row_value[i]));
        }
        for (int i = 0; i < denomination_value.size(); i++) {
            sum += (denomination_value[i] * bank_note[i]);
        }

        if (GetRowCount("collection_box") >= 1) {
            string query = "UPDATE collection_box SET thb_100 = 0, thb_20 = 0, thb_10 = 0, thb_5 = 0, thb_1 = 0 WHERE id = 1;";
            rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);
            if (rc != SQLITE_OK) {
                cerr << "SQL error: " << err_msg << endl; 
                sqlite3_free(err_msg);
            }
        } else {
            cout << "- - The collection box has no row in it." << endl;
        }
        return sum;
    }
    void RestockItem(const string item_name, const string price, const int amount) { // this method will create / 
        vector<string> all_item = GetColumnValue("stocks_67011140", "name");
        int cnt = count(all_item.begin(), all_item.end(), item_name);
        string query;

        if (cnt > 0) { // already have item in it
            string condition = "name = '" + item_name + "'";
            int previous_amount = stoi(GetRowValue("stocks_67011140", condition)[3]);
            query = "UPDATE stocks_67011140 SET price = '" + price + "', amount = " + to_string(previous_amount + amount) + " WHERE " + condition + ";";
        } else { // does not have item in the machine
            query = "INSERT INTO stocks_67011140 (name, price, amount) VALUES ('" + item_name + "', '" + price + "', " + to_string(amount) + ");";
        }
        rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK) {
            cerr << "SQL error: " << err_msg << endl; 
            sqlite3_free(err_msg);
        } else {
            cout << "- - Restock item successfully!" << endl;
        }
    }
    vector<int> GiveChange(const int price, const int receive) { // this method will do all the increase collection, decrease change, etc. about changes
        vector<int> change_given = {0, 0, 0, 0, 0}; 
        vector<string> row_value = GetRowValue("change_box", "id = 1");
        int change = receive - price;
        int total_in_change_box = 0;

        for (string s : row_value) {
            total_in_change_box += stoi(s);
        }
        if (total_in_change_box > change) {
            for (int i = 0; i < bank_note.size(); i++) {
                while (change / bank_note[i] != 0) {
                    change_given[i] += 1;
                    change -= bank_note[i];
                }
            }
        } else {
            return {0};
        }
        return change_given;
    }
    void BuyItem(const int id) { // this is the method that will reduce the amount by 1
        vector<string> row_value = GetRowValue("stocks_67011140", "id = " + to_string(id));
        int amount = stoi(row_value[3]);
        string query;

        if (amount >= 1) {
            query = "UPDATE stocks_67011140 SET amount = " + to_string(amount - 1) + " WHERE id = " + to_string(id) + ";";
            rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);
            if (rc != SQLITE_OK) {
                cerr << "SQL error: " << err_msg << endl; 
                sqlite3_free(err_msg);
            }
        } else {
            cout << "- - Sorry, the item is out of stock" << endl;
        }
    }
    bool CheckOutOfStock() { // this method will check if more than half of the table is out of stock or not, and also display out of stock for item with amount = 0
        const int total_row = GetRowCount("stocks_67011140");
        int total_out_of_stock = 0;

        if (GetRowCount("stocks_67011140") > 0) {
            for (int i = 1; i <= total_row; i++) {
                vector<string> temp_value = GetRowValue("stocks_67011140", "id = " + to_string(i));
                if (stoi(temp_value[3]) <= 0) {
                    total_out_of_stock++;
                    string query = "UPDATE stocks_67011140 SET price = 'OUT OF STOCK', amount = 0 WHERE id = " + to_string(i) + ";";
                    rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);
                    if (rc != SQLITE_OK) {
                        cerr << "SQL error: " << err_msg << endl; 
                        sqlite3_free(err_msg);
                    }
                }
            }
        }
        return total_out_of_stock >= (total_row / 2);
    } 
    bool CheckCollectionFull() { // this will check each bank note whether its reach the limit or not (1 if the machine will stop, 0 the machine will run)
        if (GetRowCount("collection_box") >= 1) {
            vector<string> row_value = GetRowValue("collection_box", "id = 1");

            for (int i = 1; i < row_value.size(); i++) {
                if (stoi(row_value[i]) >= max_collection) return 1;
            }
            return 0;
        }
        return 1;
    }
    bool CheckChangeBoxEmpty() { // this will check if any bank note in change box reach 0 or not
        if (GetRowCount("change_box") >= 1) {
            vector<string> row_value = GetRowValue("change_box", "id = 1");

            for (int i = 1; i < row_value.size(); i++) {
                if (stoi(row_value[i]) <= 0) return 1;
            }
            return 0;
        }
        return 1;
    }

public:
    VendingMachine() { // constructor
        CreateDatabase();
        SetCollectionBox();
        SetChangeBox(0,0,0,0,0);
    }
    void UserMode() {
        string user_input;
        while (true) {
            if (CheckOutOfStock() || CheckCollectionFull() || CheckChangeBoxEmpty()) { // (worked) / (worked) / (worked)
                cout << "\n- - The vending machine is not ready to use. We're terribly sorry for the inconvinience." << endl;
                break;
            }
            bool is_purchased = false;
            cout << "\n- - Welcome to this vending machine." << endl;
            PrintTable("stocks_67011140");

            while (true) { // loop until the item is selected
                cout << "\n- - What would you like to buy? (enter an ID, 0 to exit)\n> ";
                cin >> user_input;

                if (user_input == "0") break;
                int id = stoi(user_input);

                if (0 < id && id <= GetRowCount("stocks_67011140")) {
                    vector<string> row_value = GetRowValue("stocks_67011140", "id = " + to_string(id));
                    if (stoi(row_value[3]) == 0) {
                        cout << "\n- - The selected item is out of stock." << endl;
                    } else {
                        int price = stoi(row_value[2]);
                        int payment = 0;

                        while (payment < price) {
                            for (int i : {100, 20, 10, 5, 1}) {
                                int temp = 0;
                                if (i > 15) cout << "Enter " << i << " bills: ";
                                else cout << "Enter " << i << " coins: ";
                                cin >> temp;
                                payment += temp * i;
                            }
                            if (payment < price) cout << "\n- - The payment is not enough" << endl;
                        }
                        vector<int> changes = GiveChange(price, payment);
                        if (changes.size() > 1) {
                            SetChangeBox(changes[0] * -1, changes[1] * -1, changes[2] * -1, changes[3] * -1, changes[4] * -1); // next, check if the change box have enough change, cannot be negative
                            cout << "\n- - The vending machine returns some changes." << endl;
                            for (int i = 0; i < changes.size(); i++) {
                                cout << to_string(bank_note[i]) + " baht: " << changes[i] << endl;
                            }
                            BuyItem(id);
                            is_purchased = true;
                        } else {
                            cout << "\n- - The change box doesn't has enough money. Please come again later." << endl;
                        }
                        break;
                    }
                }
            }
            if (is_purchased) { // (worked)
                string user_input2;
                cout << "\n- - You have purchased an item. Would you like to make another purchase?\n1) Yes\n2) No\n> ";
                cin >> user_input2;
                if (user_input2 == "2") {
                    cout << "\n- - Thank you for using our service!" << endl;
                    break;
                }
            }
            if (user_input == "0") break; // (worked)
        }
    }
    void AdminMode() { 
        while (true) {
            string inpt = "";
            cout << "\n- - Hello, admin! What will you do?\n1) View items\n2) Set stock / Restock\n3) Check change box / collection box" << endl; 
            cout << "4) Collect money\n5) Refill change box\n0) quit\n> ";
            cin >> inpt;
            if (inpt == "1") { // print stock table (worked)
                cout << "\n- - Vending Machine's all items" << endl;
                PrintTable("stocks_67011140");
            } else if (inpt == "2") { // set stock / restock item (worked)
                string name;
                string price;
                int amount;

                cout << "\n- - Enter the item's data (name price amount)\n> ";
                ((cin >> name) >> price) >> amount;

                RestockItem(name, price, amount);
            } else if (inpt == "3") { // print change box / collection box (worked)
                if (GetRowCount("change_box") > 0) {
                    cout << "\n- - Change box information: " << endl;
                    PrintTable("change_box");
                } else {
                    cout << "- - Change box table hasn't been created yet." << endl;
                }
                if (GetRowCount("collection_box") > 0) {
                    cout << "\n- - Collection box information: " << endl;
                    PrintTable("collection_box");
                } else {
                    cout << "- - Collection box table hasn't been created yet." << endl;
                }
            } else if (inpt == "4") { // collect money from collection box (worked)
                int sum = EmptyCollection();
                cout << "\n- - You've collected " + to_string(sum) + " Baht!" << endl;
            } else if (inpt == "5") { // refill change box (worked)
                vector<int> amount_of_notes;
                for (int i = 0; i < bank_note.size(); i++) {
                    string temp;
                    cout << "\n- - Enter an amount of " + to_string(bank_note[i]) + " Baht\n> ";
                    cin >> temp;
                    amount_of_notes.push_back(stoi(temp));
                }
                SetChangeBox(amount_of_notes[0], amount_of_notes[1], amount_of_notes[2], amount_of_notes[3], amount_of_notes[4]);
            } else { // quit (worked)
                cout << "- - Have a good day, sir!" << endl;
                break;
            }
        }
    }
};

int main() {
    VendingMachine vm;
    string user;

    cout << "- - Welcome to the vending machine\n- - Type 'user' to start purchasing items\n- - Type 'admin' to enter admin mode\n> ";
    cin >> user;
 
    if (user == "admin") {
        vm.AdminMode();
    } else {
        vm.UserMode();
    }

    return 0;
}