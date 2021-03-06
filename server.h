//
// Created by albert on 24.04.18.
//

#ifndef SIK_ZADANIE_1_SERVER_H
#define SIK_ZADANIE_1_SERVER_H
#include <vector>
using namespace std;
void initialize_socket();

void check_args(int argc, char *const *argv);
bool onKeyPressed(deque<u_int8_t>& key);
struct Menu {
    vector<string> fields;
    int type=0;
    int current_field;
    const int max_field=2, min_field=0;
    Menu(const vector<string> &fields, int type) : fields(fields), type(type) {
        current_field = 0;
    }
    Menu& operator=(const Menu& other) {
        this->fields = other.fields;
        this->type = other.type;
        this->current_field = 0;
        return *this;
    }
    Menu();
};
void display(const Menu& menu, string text);

void negotiate();

void wait_for_client();
#endif //SIK_ZADANIE_1_SERVER_H
