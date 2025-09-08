
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cstdint>
#include <cstring>

using namespace std;

const int T = 64;

string to_lower(const string &s)
{
    string res = s;
    transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

class BTreeNode
{
public:
    bool leaf;
    int key_count;
    string keys[2 * T - 1];
    uint64_t values[2 * T - 1];
    BTreeNode *children[2 * T];

    BTreeNode(bool is_leaf) : leaf(is_leaf), key_count(0)
    {
        fill_n(children, 2 * T, nullptr);
    }

    ~BTreeNode()
    {
        if (!leaf)
        {
            for (int i = 0; i <= key_count; ++i)
            {
                delete children[i];
            }
        }
    }

    pair<bool, uint64_t> search(const string &k)
    {

        int lt = 0, rt = key_count - 1;
        while (lt <= rt)
        {
            int mid = (lt + rt) / 2;
            if (keys[mid] == k)
                return {true, values[mid]};
            else if (keys[mid] < k)
                lt = mid + 1;
            else
                rt = mid - 1;
        }
        if (leaf)
            return {false, 0};
        if (lt > key_count || children[lt] == nullptr)
            return {false, 0};
        return children[lt]->search(k);
    }

    void insert_non_full(const string &k, uint64_t v)
    {
        if (leaf)
        {
            int pos = key_count - 1;
            while (pos >= 0 && keys[pos] > k)
            {
                keys[pos + 1] = keys[pos];
                values[pos + 1] = values[pos];
                pos--;
            }
            keys[pos + 1] = k;
            values[pos + 1] = v;
            key_count++;
        }
        else
        {
            int pos = key_count - 1;
            while (pos >= 0 && keys[pos] > k)
                pos--;
            pos++;
            if (children[pos]->key_count == 2 * T - 1)
            {
                split_child(pos);
                if (keys[pos] < k)
                    pos++;
            }
            children[pos]->insert_non_full(k, v);
        }
    }

    void split_child(int i)
    {
        BTreeNode *y = children[i];
        BTreeNode *z = new BTreeNode(y->leaf);
        z->key_count = T - 1;

        for (int j = 0; j < T - 1; ++j)
        {
            z->keys[j] = y->keys[j + T];
            z->values[j] = y->values[j + T];
        }

        if (!y->leaf)
        {
            for (int j = 0; j < T; ++j)
                z->children[j] = y->children[j + T];
        }
        y->key_count = T - 1;
        for (int j = key_count; j >= i + 1; --j)
            children[j + 1] = children[j];
        children[i + 1] = z;
        for (int j = key_count - 1; j >= i; --j)
        {
            keys[j + 1] = keys[j];
            values[j + 1] = values[j];
        }

        keys[i] = y->keys[T - 1];
        values[i] = y->values[T - 1];
        key_count++;
    }

    bool remove(const string &k)
    {
        int idx = 0;
        while (idx < key_count && keys[idx] < k)
            ++idx;
        if (idx < key_count && keys[idx] == k)
        {
            if (leaf)
            {
                remove_from_leaf(idx);

                return true;
            }
            return remove_from_non_leaf(idx);
        }
        else
        {
            if (leaf)
                return false;
            bool flag = (idx == key_count);
            if (children[idx]->key_count < T)
                fill(idx);
            if (flag && idx > key_count)
                idx--;
            return children[idx]->remove(k);
        }
    }

    void remove_from_leaf(int idx)
    {
        for (int i = idx + 1; i < key_count; ++i)
        {
            keys[i - 1] = keys[i];
            values[i - 1] = values[i];
        }
        key_count--;
    }

    bool remove_from_non_leaf(int idx)
    {
        string k = keys[idx];
        if (children[idx]->key_count >= T)
        {
            auto pred = get_pred(idx);
            keys[idx] = pred.first;
            values[idx] = pred.second;
            return children[idx]->remove(pred.first);
        }
        else if (children[idx + 1]->key_count >= T)
        {
            auto succ = get_succ(idx);
            keys[idx] = succ.first;
            values[idx] = succ.second;
            return children[idx + 1]->remove(succ.first);
        }
        else
        {
            merge(idx);
            return children[idx]->remove(k);
        }
    }

    pair<string, uint64_t> get_pred(int idx)
    {
        BTreeNode *cur = children[idx];
        while (!cur->leaf)
            cur = cur->children[cur->key_count];
        return {cur->keys[cur->key_count - 1], cur->values[cur->key_count - 1]};
    }
    pair<string, uint64_t> get_succ(int idx)
    {
        BTreeNode *cur = children[idx + 1];
        while (!cur->leaf)
            cur = cur->children[0];
        return {cur->keys[0], cur->values[0]};
    }

    void fill(int idx)
    {
        if (idx != 0 && children[idx - 1]->key_count >= T)
            borrow_from_prev(idx);
        else if (idx != key_count && children[idx + 1]->key_count >= T)
            borrow_from_next(idx);
        else
        {
            if (idx != key_count)
                merge(idx);
            else
                merge(idx - 1);
        }
    }

    void borrow_from_prev(int idx)
    {
        BTreeNode *child = children[idx];
        BTreeNode *sibling = children[idx - 1];

        for (int i = child->key_count - 1; i >= 0; --i)
        {
            child->keys[i + 1] = child->keys[i];
            child->values[i + 1] = child->values[i];
        }
        if (!child->leaf)
        {
            for (int i = child->key_count; i >= 0; --i)
                child->children[i + 1] = child->children[i];
        }

        child->keys[0] = keys[idx - 1];
        child->values[0] = values[idx - 1];
        if (!child->leaf)
            child->children[0] = sibling->children[sibling->key_count];

        keys[idx - 1] = sibling->keys[sibling->key_count - 1];
        values[idx - 1] = sibling->values[sibling->key_count - 1];
        child->key_count++;
        sibling->key_count--;
    }

    void borrow_from_next(int idx)
    {
        BTreeNode *child = children[idx];
        BTreeNode *sibling = children[idx + 1];

        child->keys[child->key_count] = keys[idx];
        child->values[child->key_count] = values[idx];
        if (!child->leaf)
            child->children[child->key_count + 1] = sibling->children[0];

        keys[idx] = sibling->keys[0];
        values[idx] = sibling->values[0];

        for (int i = 1; i < sibling->key_count; ++i)
        {
            sibling->keys[i - 1] = sibling->keys[i];
            sibling->values[i - 1] = sibling->values[i];
        }
        if (!sibling->leaf)
        {
            for (int i = 1; i <= sibling->key_count; ++i)
                sibling->children[i - 1] = sibling->children[i];
        }
        child->key_count++;
        sibling->key_count--;
    }

    void merge(int idx)
    {
        BTreeNode *child = children[idx];
        BTreeNode *sibling = children[idx + 1];

        child->keys[T - 1] = keys[idx];

        child->values[T - 1] = values[idx];

        for (int i = 0; i < sibling->key_count; ++i)
        {
            child->keys[i + T] = sibling->keys[i];
            child->values[i + T] = sibling->values[i];
        }

        if (!child->leaf)
        {
            for (int i = 0; i <= sibling->key_count; ++i)
                child->children[i + T] = sibling->children[i];
        }

        for (int i = idx + 1; i < key_count; ++i)
        {
            keys[i - 1] = keys[i];
            values[i - 1] = values[i];
        }
        for (int i = idx + 2; i <= key_count; ++i)
            children[i - 1] = children[i];
        child->key_count += sibling->key_count + 1;
        key_count--;

        sibling->leaf = true;
        for (int i = 0; i <= sibling->key_count; ++i)
            sibling->children[i] = nullptr;
        delete sibling;
    }

    bool serialize(ofstream &out)
    {
        out.write(reinterpret_cast<char *>(&leaf), sizeof(leaf));
        out.write(reinterpret_cast<char *>(&key_count), sizeof(key_count));
        for (int i = 0; i < key_count; ++i)
        {
            size_t len = keys[i].size();
            out.write(reinterpret_cast<char *>(&len), sizeof(len));
            out.write(keys[i].data(), len);
            out.write(reinterpret_cast<char *>(&values[i]), sizeof(values[i]));
        }
        if (!leaf)
        {
            for (int i = 0; i <= key_count; ++i)
            {
                if (!children[i]->serialize(out))
                    return false;
            }
        }
        return true;
    }

    static BTreeNode *deserialize(ifstream &in)
    {
        bool leaf;
        if (!in.read(reinterpret_cast<char *>(&leaf), sizeof(leaf)))
            return nullptr;
        BTreeNode *node = new BTreeNode(leaf);
        in.read(reinterpret_cast<char *>(&node->key_count), sizeof(node->key_count));
        for (int i = 0; i < node->key_count; ++i)
        {
            size_t len;
            in.read(reinterpret_cast<char *>(&len), sizeof(len));
            node->keys[i].resize(len);
            in.read(&node->keys[i][0], len);
            in.read(reinterpret_cast<char *>(&node->values[i]), sizeof(node->values[i]));
        }
        if (!leaf)
        {
            for (int i = 0; i <= node->key_count; ++i)
            {
                node->children[i] = deserialize(in);
            }
        }
        return node;
    }
};

class BTree
{
public:
    BTreeNode *root;
    BTree() : root(new BTreeNode(true)) {}
    ~BTree() { delete root; }

    bool insert(const string &word, uint64_t val)
    {
        if (root->search(word).first)
            return false;
        if (root->key_count == 2 * T - 1)
        {
            BTreeNode *new_root = new BTreeNode(false);
            new_root->children[0] = root;
            new_root->split_child(0);

            int i = (new_root->keys[0] < word ? 1 : 0);
            new_root->children[i]->insert_non_full(word, val);
            root = new_root;
        }
        else
        {
            root->insert_non_full(word, val);
        }
        return true;
    }

    bool remove(const string &word)
    {
        if (!root->key_count)
            return false;
        bool res = root->remove(word);
        if (root->key_count == 0 && !root->leaf)
        {
            BTreeNode *old = root;
            root = root->children[0];
            old->children[0] = nullptr;
            delete old;
        }
        return res;
    }

    pair<bool, uint64_t> search(const string &word)
    {
        if (!root)
            return {false, 0};
        return root->search(word);
    }

    bool save(const string &path, string &err)
    {
        ofstream out(path, ios::binary);
        if (!out)
        {
            err = "Cannot open file";
            return false;
        }
        if (!root->serialize(out))
        {
            err = "Serialize error";
            return false;
        }
        return true;
    }

    bool load(const string &path, string &err)
    {
        ifstream in(path, ios::binary);
        if (!in)
        {
            err = "Cannot open file";
            return false;
        }
        BTreeNode *nr = BTreeNode::deserialize(in);
        if (!nr)
        {
            err = "Deserialize error";
            return false;
        }
        delete root;
        root = nr;

        return true;
    }
};

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    BTree tree;
    string line;
    while (getline(cin, line))
    {
        if (line.empty())
            continue;
        if (line[0] == '+')
        {
            istringstream iss(line);
            string cmd, word;
            uint64_t val;
            iss >> cmd >> word >> val;
            word = to_lower(word);
            cout << (tree.insert(word, val) ? "OK" : "Exist") << '\n';
        }
        else if (line[0] == '-')
        {
            string word = to_lower(line.substr(2));
            cout << (tree.remove(word) ? "OK" : "NoSuchWord") << '\n';
        }
        else if (line[0] == '!')
        {
            istringstream iss(line);
            string bang, cmd, path;
            iss >> bang >> cmd >> path;
            string err;
            if (cmd == "Save")
                cout << (tree.save(path, err) ? "OK" : "ERROR: " + err) << '\n';
            else if (cmd == "Load")
                cout << (tree.load(path, err) ? "OK" : "ERROR: " + err) << '\n';
        }
        else
        {
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