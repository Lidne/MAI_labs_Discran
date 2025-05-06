#include <bits/stdc++.h>
using namespace std;

class Treap {
    struct Node {
        string key;
        uint64_t val;
        uint32_t pr;
        Node *l, *r;
        Node(const string& key, uint64_t val, uint32_t pr)
            : key(key), val(val), pr(pr), l(nullptr), r(nullptr) {}
    };

    Node* root;
    mt19937 rng;

    Node* merge(Node* a, Node* b) {
        if (!a) return b;
        if (!b) return a;
        if (a->pr > b->pr) {
            a->r = merge(a->r, b);
            return a;
        } else {
            b->l = merge(a, b->l);
            return b;
        }
    }

    void split(Node* t, const string& key, Node*& a, Node*& b) {
        if (!t) {
            a = b = nullptr;
        } else if (t->key < key) {
            split(t->r, key, t->r, b);
            a = t;
        } else {
            split(t->l, key, a, t->l);
            b = t;
        }
    }

    Node* insert(Node* t, Node* it, bool& inserted) {
        if (!t) {
            inserted = true;
            return it;
        }
        if (it->key == t->key) {
            inserted = false;
            delete it;
            return t;
        }
        if (it->pr < t->pr) {
            split(t, it->key, it->l, it->r);
            inserted = true;
            return it;
        } else if (it->key < t->key) {
            t->l = insert(t->l, it, inserted);
        } else {
            t->r = insert(t->r, it, inserted);
        }
        return t;
    }

    Node* erase(Node* t, const string& key, bool& erased) {
        if (!t) {
            erased = false;
            return nullptr;
        }
        if (t->key == key) {
            erased = true;
            Node* res = merge(t->l, t->r);
            delete t;
            return res;
        } else if (key < t->key) {
            t->l = erase(t->l, key, erased);
        } else {
            t->r = erase(t->r, key, erased);
        }
        return t;
    }

    Node* find(Node* t, const string& key) const {
        if (!t) return nullptr;
        if (t->key == key) return t;
        if (key < t->key)
            return find(t->l, key);
        else
            return find(t->r, key);
    }

   public:
    Treap() : root(nullptr), rng(random_device{}()) {}

    bool insert(const string& key, uint64_t val) {
        Node* it = new Node(key, val, rng());
        bool inserted = false;
        root = insert(root, it, inserted);
        return inserted;
    }

    bool erase(const string& key) {
        bool erased = false;
        root = erase(root, key, erased);
        return erased;
    }

    bool find(const string& key, uint64_t& val) const {
        Node* res = find(root, key);
        if (res) {
            val = res->val;
            return true;
        }
        return false;
    }
};

static inline string toLower(const string& s) {
    string t = s;
    for (char& c : t) {
        c = tolower(static_cast<unsigned char>(c));
    }
    return t;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Treap treap;
    string line;
    while (cin >> line) {
        if (line == "+") {
            string word;
            uint64_t num;
            cin >> word >> num;
            word = toLower(word);
            uint64_t some;
            if (treap.find(word, some)) {
                cout << "Exist\n";
            } else {
                if (treap.insert(word, num))
                    cout << "OK\n";
                else
                    cout << "Exist\n";
            }
        } else if (line == "-") {
            string word;
            cin >> word;
            word = toLower(word);
            if (treap.erase(word))
                cout << "OK\n";
            else
                cout << "NoSuchWord\n";
        } else {
            string word = toLower(line);
            uint64_t num;
            if (treap.find(word, num))
                cout << "OK: " << num << "\n";
            else
                cout << "NoSuchWord\n";
        }
    }

    return 0;
}
