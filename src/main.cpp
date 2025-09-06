#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

const int T = 64;

string to_lower(const string &o) {
    string w = o;
    for (size_t h = 0; h < w.length(); ++h) w[h] = tolower(w[h]);
    return w;
}

class BNode {
   public:
    bool leaf;
    int c;
    string keys[2 * T - 1];
    uint64_t values[2 * T - 1];
    BNode *children[2 * T];

    BNode(bool leaf_) : leaf(leaf_), c(0) {
        std::fill(children, children + 2 * T, nullptr);
    }

    ~BNode() {
        if (!leaf) {
            for (int j = 0; j <= c; ++j) {
                delete children[j];
            }
        }
    }

    pair<bool, uint64_t> search(const string &k) {
        int l = 0, r = c - 1;
        while (l <= r) {
            int m = l + (r - l) / 2;
            if (keys[m] == k)
                return {true, values[m]};
            else if (keys[m] < k)
                l = m + 1;
            else
                r = m - 1;
        }
        if (leaf)
            return {false, 0};
        if (l > c || children[l] == nullptr)
            return {false, 0};
        return children[l]->search(k);
    }

    void insertNotNull(const string &k, uint64_t v) {
        int idx = c - 1;

        if (leaf) {
            while (idx >= 0 && keys[idx] > k) {
                keys[idx + 1] = keys[idx];
                values[idx + 1] = values[idx];
                --idx;
            }
            keys[idx + 1] = k;
            values[idx + 1] = v;
            c++;
        } else {
            while (idx >= 0 && keys[idx] > k)
                idx--;
            idx++;
            if (children[idx]->c == 2 * T - 1) {
                split(idx);
                if (keys[idx] < k)
                    idx++;
            }
            children[idx]->insertNotNull(k, v);
        }
    }

    void split(int index) {
        BNode *origin = children[index];
        BNode *newSibling = new BNode(origin->leaf);
        newSibling->c = T - 1;

        for (int i = 0; i < T - 1; ++i) {
            newSibling->keys[i] = origin->keys[i + T];
            newSibling->values[i] = origin->values[i + T];
        }
        if (!origin->leaf) {
            for (int i = 0; i < T; ++i)
                newSibling->children[i] = origin->children[i + T];
        }

        origin->c = T - 1;

        for (int i = c; i >= index + 1; --i)
            children[i + 1] = children[i];
        children[index + 1] = newSibling;

        for (int i = c - 1; i >= index; --i) {
            keys[i + 1] = keys[i];
            values[i + 1] = values[i];
        }
        keys[index] = origin->keys[T - 1];
        values[index] = origin->values[T - 1];
        c++;
    }

    bool remove(const string &key) {
        int idx = 0;
        while (idx < c && keys[idx] < key)
            idx++;
        if (idx < c && keys[idx] == key) {
            if (leaf) {
                drop(idx);
                return true;
            }
            return cull(idx);
        } else {
            if (leaf)
                return false;
            bool at_end = (idx == c);
            if (children[idx]->c < T)
                fill(idx);
            if (at_end && idx > c)
                idx--;
            return children[idx]->remove(key);
        }
    }

    void drop(int idx) {
        for (int j = idx + 1; j < c; ++j) {
            keys[j - 1] = keys[j];
            values[j - 1] = values[j];
        }
        c--;
    }

    bool cull(int idx) {
        string delKey = keys[idx];
        if (children[idx]->c >= T) {
            auto pred = getPred(idx);
            keys[idx] = pred.first;
            values[idx] = pred.second;
            return children[idx]->remove(pred.first);
        } else if (children[idx + 1]->c >= T) {
            auto succ = getSucc(idx);
            keys[idx] = succ.first;
            values[idx] = succ.second;
            return children[idx + 1]->remove(succ.first);
        } else {
            merge(idx);
            return children[idx]->remove(delKey);
        }
    }

    pair<string, uint64_t> getPred(int t) {
        BNode *cur = children[t];
        while (!cur->leaf)
            cur = cur->children[cur->c];
        return {cur->keys[cur->c - 1], cur->values[cur->c - 1]};
    }

    pair<string, uint64_t> getSucc(int t) {
        BNode *cur = children[t + 1];
        while (!cur->leaf)
            cur = cur->children[0];
        return {cur->keys[0], cur->values[0]};
    }

    void fill(int idx) {
        if (idx != 0 && children[idx - 1]->c >= T)
            siphonL(idx);
        else if (idx != c && children[idx + 1]->c >= T)
            siphonR(idx);
        else {
            if (idx != c)
                merge(idx);
            else
                merge(idx - 1);
        }
    }

    void siphonL(int idx) {
        BNode *curChild = children[idx];
        BNode *leftSibling = children[idx - 1];
        for (int i = curChild->c - 1; i >= 0; --i) {
            curChild->keys[i + 1] = curChild->keys[i];
            curChild->values[i + 1] = curChild->values[i];
        }
        if (!curChild->leaf) {
            for (int i = curChild->c; i >= 0; --i)
                curChild->children[i + 1] = curChild->children[i];
        }
        curChild->keys[0] = keys[idx - 1];
        curChild->values[0] = values[idx - 1];
        if (!curChild->leaf)
            curChild->children[0] = leftSibling->children[leftSibling->c];

        keys[idx - 1] = leftSibling->keys[leftSibling->c - 1];
        values[idx - 1] = leftSibling->values[leftSibling->c - 1];
        curChild->c++;
        leftSibling->c--;
    }

    void siphonR(int idx) {
        BNode *child = children[idx];
        BNode *rightSibling = children[idx + 1];

        child->keys[child->c] = keys[idx];
        child->values[child->c] = values[idx];
        if (!child->leaf)
            child->children[child->c + 1] = rightSibling->children[0];

        keys[idx] = rightSibling->keys[0];
        values[idx] = rightSibling->values[0];

        for (int i = 1; i < rightSibling->c; ++i) {
            rightSibling->keys[i - 1] = rightSibling->keys[i];
            rightSibling->values[i - 1] = rightSibling->values[i];
        }
        if (!rightSibling->leaf) {
            for (int i = 1; i <= rightSibling->c; ++i)
                rightSibling->children[i - 1] = rightSibling->children[i];
        }
        child->c++;
        rightSibling->c--;
    }

    void merge(int idx) {
        BNode *leftChild = children[idx];
        BNode *rightChild = children[idx + 1];

        leftChild->keys[T - 1] = keys[idx];
        leftChild->values[T - 1] = values[idx];

        for (int i = 0; i < rightChild->c; ++i) {
            leftChild->keys[i + T] = rightChild->keys[i];
            leftChild->values[i + T] = rightChild->values[i];
        }
        if (!leftChild->leaf) {
            for (int i = 0; i <= rightChild->c; ++i)
                leftChild->children[i + T] = rightChild->children[i];
        }
        for (int i = idx + 1; i < c; ++i) {
            keys[i - 1] = keys[i];
            values[i - 1] = values[i];
        }
        for (int i = idx + 2; i <= c; ++i)
            children[i - 1] = children[i];

        leftChild->c += rightChild->c + 1;
        c--;
        rightChild->leaf = true;
        for (int i = 0; i <= rightChild->c; ++i)
            rightChild->children[i] = nullptr;
        delete rightChild;
    }

    bool dump(ofstream &fo) {
        fo.write((char *)(&leaf), sizeof(leaf));
        fo.write((char *)(&c), sizeof(c));
        for (int i = 0; i < c; ++i) {
            size_t slen = keys[i].size();
            fo.write((char *)(&slen), sizeof(slen));
            fo.write(keys[i].data(), slen);
            fo.write((char *)&values[i], sizeof(values[i]));
        }
        if (!leaf) {
            for (int i = 0; i <= c; ++i) {
                if (!children[i]->dump(fo))
                    return false;
            }
        }
        return true;
    }

    static BNode *load(ifstream &fi) {
        bool leaf_status;
        if (!fi.read((char *)&leaf_status, sizeof(bool)))
            return nullptr;

        BNode *node = new BNode(leaf_status);
        fi.read((char *)(&node->c), sizeof(node->c));
        for (int i = 0; i < node->c; ++i) {
            size_t len;
            fi.read((char *)&len, sizeof(len));
            node->keys[i].resize(len);
            fi.read(&node->keys[i][0], len);
            fi.read((char *)&node->values[i], sizeof(node->values[i]));
        }
        if (!leaf_status) {
            for (int i = 0; i <= node->c; ++i) {
                node->children[i] = load(fi);
            }
        }
        return node;
    }
};

class BTree {
   public:
    BNode *root;
    BTree() : root(new BNode(true)) {}
    ~BTree() { delete root; }

    bool add(const string &word, uint64_t val) {
        if (root->search(word).first)
            return false;

        if (root->c == 2 * T - 1) {
            BNode *newRoot = new BNode(false);
            newRoot->children[0] = root;
            newRoot->split(0);

            int childIndex = (newRoot->keys[0] < word) ? 1 : 0;
            newRoot->children[childIndex]->insertNotNull(word, val);

            root = newRoot;
        } else {
            root->insertNotNull(word, val);
        }
        return true;
    }

    bool remove(const string &v) {
        if (!root->c)
            return false;
        bool found = root->remove(v);
        if (root->c == 0 && !root->leaf) {
            BNode *oldRoot = root;
            root = root->children[0];
            oldRoot->children[0] = nullptr;
            delete oldRoot;
        }
        return found;
    }

    pair<bool, uint64_t> search(const string &word) {
        if (!root)
            return {false, 0};
        return root->search(word);
    }

    bool dump(const string &filename, string &errmsg) {
        ofstream outFile(filename, std::ios::binary);
        if (!outFile) {
            errmsg = "Cannot open file";
            return false;
        }
        if (!root->dump(outFile)) {
            errmsg = "Serialize error";
            return false;
        }
        return true;
    }

    bool load(const string &fname, string &errmsg) {
        ifstream inFile(fname, std::ios::binary);
        if (!inFile) {
            errmsg = "Cannot open file";
            return false;
        }
        BNode *newRoot = BNode::load(inFile);
        if (!newRoot) {
            errmsg = "Deserialize error";
            return false;
        }
        delete root;
        root = newRoot;
        return true;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    cin.tie(nullptr);

    BTree tree;
    string line;

    while (getline(cin, line)) {
        if (line.empty())
            continue;
        if (line[0] == '+') {
            istringstream iss(line);
            string cmd, word;
            uint64_t val;
            iss >> cmd >> word >> val;
            word = to_lower(word);
            cout << (tree.add(word, val) ? "OK" : "Exist") << '\n';
        } else if (line[0] == '-') {
            string word = to_lower(line.substr(2));
            cout << (tree.remove(word) ? "OK" : "NoSuchWord") << '\n';
        } else if (line[0] == '!') {
            istringstream iss(line);
            string bang, cmd, path;
            iss >> bang >> cmd >> path;
            string err;
            if (cmd == "Save")
                cout << (tree.dump(path, err) ? "OK" : "ERROR: " + err) << '\n';
            else if (cmd == "Load")
                cout << (tree.load(path, err) ? "OK" : "ERROR: " + err) << '\n';
        } else {
            string word = to_lower(line);
            auto res = tree.search(word);
            if (res.first)
                cout << "OK: " << res.second << '\n';
            else
                cout << "NoSuchWord\n";
        }
    }
    return 0;
}