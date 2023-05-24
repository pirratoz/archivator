#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <list>

using std::locale;
using std::list;
using std::string;
using std::cout;
using std::cin;
using std::endl;
using std::memset;
using std::ofstream;
using std::ifstream;


struct Node{
    unsigned char ch;
    unsigned int count;
    unsigned int count_bit;
    unsigned int bit_;
    string bit__ = "";
    Node *left = nullptr;
    Node *right = nullptr;
    Node(unsigned char ch, unsigned int count){
        this->ch = ch;
        this->count = count;
        this->count_bit = 0;
        this->bit_ = 0;
    };
    Node(unsigned char ch, unsigned int count_bit, unsigned int bit_){
        this->ch = ch;
        this->count = 0;
        this->count_bit = count_bit;
        this->bit_ = bit_;
    };
};


void fill_bits(const string & path, int bits[256]){
    ifstream file(path, std::ios::binary);
    unsigned char s = 0;
    char symbol = 0;
    while (file.get(symbol)) {
        s = static_cast<unsigned char>(symbol);
        bits[s] += 1;
    }
    file.close();
}


void recursion(Node* branch, list<Node*> & array, unsigned int bit_, unsigned int sb){
    if (branch->left == nullptr && branch->right == nullptr) {
        for (auto i : array){
            if (i->ch == branch->ch){
                branch->bit_ = bit_;
                branch->count_bit = sb;
                return;
            }
        }
        return;
    }

    recursion(branch->left, array, (bit_ << 1), sb + 1);
    recursion(branch->right, array, (bit_ << 1) + 1, sb + 1);
}


void create_root_from_array(list<Node*> & array){
    while (array.size() != 1)
    {
        array.sort([](Node* f, Node* s){return (f->count > s->count);});
        auto f = array.back();
        array.pop_back();
        auto s = array.back();
        array.pop_back();
        auto t = new Node(0, f->count + s->count);
        t->left = f;
        t->right = s;
        array.push_back(t);
    }
}

list<Node*> create_tree(int bits[256]){
    list<Node*> array;
    list<Node*> array2;
    for (int i = 0; i < 256; ++i){
        if (bits[i] > 0) array.push_back(new Node((unsigned char)i, bits[i]));
    }
    if (array.size() == 1){
        array.front()->count_bit = 1;
        return array;
    }
    array2 = array;
    create_root_from_array(array);
    recursion(array.front(), array2, 0, 0);
    // for (auto i : array2){
    //     cout << i->ch << " " << i->bit_ << " " << i->count_bit << endl;
    // }
    return array2;
}


unsigned get_sum_bits(const int bits[256]){
    unsigned sum = 0;
    for (int i = 0; i < 256; ++i) sum += bits[i];
    return sum;
}


void encode(const string & path, int bits[256], list<Node*> array){
    unsigned count_symbols = get_sum_bits(bits);
    unsigned code_alp = 0;
    ofstream f_code(path + ".codePTZ", std::ios::binary);
    locale oem_locale("ru_RU.utf-8");
    f_code.imbue(oem_locale);
    f_code.write((char*)&count_symbols, 4);
    code_alp = 4 + (array.size() * 5);
    for (auto i : array){
        f_code.write((char*)&i->ch, 1);
        f_code.write((char*)&i->count, 4);
    }
    f_code.close();

    ifstream f_text(path, std::ios::binary);
    ofstream f_zip(path + ".textPTZ", std::ios::binary);
    f_zip.imbue(oem_locale);
    
    unsigned b = 0;
    unsigned bs = 0;
    unsigned symbol = 0;
    char s;
    float encoded_chars = 0;

    float ss = 0;
    while (f_text.get(s)){
        symbol = ((s >= 0) ? s : 256 + s);
        for (auto i : array){
            if (i->ch == symbol){
                bs += i->count_bit;
                b = (b << i->count_bit) + i->bit_;
                break;
            }
        }
        while (bs >= 8)
        {
            char f = (char)(b >> (bs - 8));
            f_zip.write(&f, 1);
            b = (b << (32 - (bs - 8))) >> (32 - (bs - 8));
            bs -= 8;
            ss += 8;
            encoded_chars += 1;
        }
    }
    if (bs > 0){
        ss += bs;
        char f = (char)(b << (8 - bs));
        encoded_chars += 1;
        f_zip.write(&f, 1);
    }
    f_code.close();
    f_zip.close();
    cout << "Старый размер файла: " << count_symbols << " байт" << endl;
    cout << "Новый размер файла: " << encoded_chars << " байт" << endl;
    cout << "zip: " << 1 - (encoded_chars / count_symbols) << " %" << endl;
}


void decode(const string & path, int bits[256]){
    unsigned count_symbols = 0;
    unsigned char symbol = 0;
    unsigned count = 0;
    ifstream f_code(path + ".codePTZ", std::ios::binary);
    f_code.read(reinterpret_cast<char*>(&count_symbols), 4);
    while (!f_code.eof()){
        f_code.read(reinterpret_cast<char*>(&symbol), 1);
        f_code.read(reinterpret_cast<char*>(&count), 4);
        bits[symbol] = count;
    }
    f_code.close();
    list<Node*> array = create_tree(bits);
    // cout << "===================================" << "\n";
    // for (auto i : array){
    //     cout << std::to_string(static_cast<int>(i->ch)) << " " << i->bit_ << " " << i->count_bit << "\n";
    // }

    
    ifstream f_text(path + ".textPTZ", std::ios::binary);
    ofstream f_unzip(path + ".unPTZ", std::ios::binary);
    locale oem_locale("ru_RU.utf-8");
    f_unzip.imbue(oem_locale);

    unsigned b = 0;
    unsigned bs = 0;
    unsigned counter = 1;
    char s;

    while (f_text.get(s)){
        symbol = ((s >= 0) ? s : 256 + s);
        b = (b << 8) + symbol;
        bs += 8;
        // cout << "BIN: " << b << " " << bs << "\n";
        while (counter--)
        {
            for (auto i : array){
                if (count_symbols == 0 || bs == 0) break;
                if (i->count_bit > bs) continue;
                // cout << "if (" << i->bit_ << " " << (b >> (bs - i->count_bit)) << ")" << endl;
                if (i->bit_ != (b >> (bs - i->count_bit))) continue;
                bs -= i->count_bit;
                if (bs == 0) b = 0;
                else b = (b << (32 - bs)) >> (32 - bs);
                //cout << "BIN(E): " << b << " " << bs << "\n";
                counter++;
                f_unzip.write((char *)&i->ch, 1);
                count_symbols -= 1;
            }
        }
        counter = 1;
    }
    f_unzip.close();
    f_text.close();
}


void unzip(const string & path){
    auto begin = std::chrono::high_resolution_clock::now();
    int bits[256];
    memset(bits, 0, 256 * 4);
    decode(path, bits);
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed_ns = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
    cout << "Время разжатия: " << elapsed_ns << " seconds" << endl;
}


void zip(const string & path){
    auto begin = std::chrono::high_resolution_clock::now();
    int bits[256];
    memset(bits, 0, 256 * 4);
    fill_bits(path, bits);
    list<Node*> array = create_tree(bits);
    // for (auto i : array){
    //     cout << std::to_string(static_cast<int>(i->ch)) << " " << i->bit_ << " " << i->count_bit << "\n";
    // }
    encode(path, bits, array);
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed_ns = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
    cout << "Время сжатия: " << elapsed_ns  << " seconds" << endl;
}


void help_menu(){
    cout << "Доступные команды:\n"
            << "0 - выход из программы\n"
            << "1 - сжать файл\n"
            << "2 - разжать файл\n"
            << "3 - помощь\n";
}


int main(){
    setlocale(0, "ru_RU.utf-8");
    char menu = '-';
    string path;
    help_menu();
    while (menu != '0'){
        cout << "Команда: "; cin >> menu;
        if (menu == '1') {
            cout << "Введите путь к файлу: "; cin >> path;
            zip(path);
        } else if (menu == '2'){
            cout << "Введите путь к файлу: "; cin >> path;
            unzip(path);
        } else if (menu == '3'){
            help_menu();
        }
    }
    return 0;
}