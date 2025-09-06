#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

const int T = 64;

static string lowercase_copy(const string &src) {
    string result = src;
    transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

class BNode {
   public:
    bool isLeaf;
    int count;
    string keySlots[2 * T - 1];
    uint64_t valSlots[2 * T - 1];
    BNode *childPtrs[2 * T];

    explicit BNode(bool leafFlag) : isLeaf(leafFlag), count(0) {
        std::fill(childPtrs, childPtrs + 2 * T, nullptr);
    }

    ~BNode() {
        if (!isLeaf) {
            for (int j = 0; j <= count; ++j) {
                delete childPtrs[j];
            }
        }
    }

    pair<bool, uint64_t> search(const string &key) {
        int pos = findPosition(key);
        if (isMatchAt(pos, key)) {
            return {true, valSlots[pos]};
        }
        if (isLeaf) {
            return notFound();
        }
        if (isInvalidChild(pos)) {
            return notFound();
        }
        return descendSearch(pos, key);
    }

    void insertIntoNonFull(const string &key, uint64_t value) {
        if (isLeaf) {
            insertIntoLeafAtSorted(key, value);
            return;
        }
        int idx = findInsertChildIndex(key);
        ensureChildWritable(idx, key);
        childPtrs[idx]->insertIntoNonFull(key, value);
    }

    void splitChild(int index) {
        BNode *origin = childPtrs[index];
        BNode *sibling = allocateSibling(origin);
        moveKeysToSibling(origin, sibling);
        moveChildrenToSibling(origin, sibling);
        origin->count = T - 1;
        insertSiblingIntoParent(index, sibling);
        liftMedianFromOrigin(index, origin);
    }

    bool remove(const string &key) {
        int idx = findPosition(key);
        if (isMatchAt(idx, key)) {
            return removeWhenFound(idx);
        }
        if (isLeaf) {
            return false;
        }
        bool wasRight = (idx == count);
        prepareChildForRemoval(idx);
        idx = adjustIndexAfterRebalance(idx, wasRight);
        return childPtrs[idx]->remove(key);
    }

    void eraseAtIndex(int idx) {
        for (int j = idx + 1; j < count; ++j) {
            keySlots[j - 1] = keySlots[j];
            valSlots[j - 1] = valSlots[j];
        }
        --count;
    }

    bool removeFromInternal(int idx) {
        string keyToDelete = keySlots[idx];
        if (childPtrs[idx]->count >= T) {
            auto pred = findPredecessor(idx);
            keySlots[idx] = pred.first;
            valSlots[idx] = pred.second;
            return childPtrs[idx]->remove(pred.first);
        }
        if (childPtrs[idx + 1]->count >= T) {
            auto succ = findSuccessor(idx);
            keySlots[idx] = succ.first;
            valSlots[idx] = succ.second;
            return childPtrs[idx + 1]->remove(succ.first);
        }

        mergeChildren(idx);
        return childPtrs[idx]->remove(keyToDelete);
    }

    pair<string, uint64_t> findPredecessor(int t) {
        BNode *cur = childPtrs[t];
        while (!cur->isLeaf) {
            cur = cur->childPtrs[cur->count];
        }
        return {cur->keySlots[cur->count - 1], cur->valSlots[cur->count - 1]};
    }

    pair<string, uint64_t> findSuccessor(int t) {
        BNode *cur = childPtrs[t + 1];
        while (!cur->isLeaf) {
            cur = cur->childPtrs[0];
        }
        return {cur->keySlots[0], cur->valSlots[0]};
    }

    void rebalanceChild(int idx) {
        if (idx != 0 && childPtrs[idx - 1]->count >= T) {
            borrowFromLeft(idx);
            return;
        }
        if (idx != count && childPtrs[idx + 1]->count >= T) {
            borrowFromRight(idx);
            return;
        }
        if (idx != count) {
            mergeChildren(idx);
        } else {
            mergeChildren(idx - 1);
        }
    }

    void borrowFromLeft(int idx) {
        BNode *child = childPtrs[idx];
        BNode *left = childPtrs[idx - 1];

        for (int i = child->count - 1; i >= 0; --i) {
            child->keySlots[i + 1] = child->keySlots[i];
            child->valSlots[i + 1] = child->valSlots[i];
        }
        if (!child->isLeaf) {
            for (int i = child->count; i >= 0; --i) {
                child->childPtrs[i + 1] = child->childPtrs[i];
            }
        }
        child->keySlots[0] = keySlots[idx - 1];
        child->valSlots[0] = valSlots[idx - 1];
        if (!child->isLeaf) {
            child->childPtrs[0] = left->childPtrs[left->count];
        }

        keySlots[idx - 1] = left->keySlots[left->count - 1];
        valSlots[idx - 1] = left->valSlots[left->count - 1];
        ++child->count;
        --left->count;
    }

    void borrowFromRight(int idx) {
        BNode *child = childPtrs[idx];
        BNode *right = childPtrs[idx + 1];

        child->keySlots[child->count] = keySlots[idx];
        child->valSlots[child->count] = valSlots[idx];
        if (!child->isLeaf) {
            child->childPtrs[child->count + 1] = right->childPtrs[0];
        }

        keySlots[idx] = right->keySlots[0];
        valSlots[idx] = right->valSlots[0];

        for (int i = 1; i < right->count; ++i) {
            right->keySlots[i - 1] = right->keySlots[i];
            right->valSlots[i - 1] = right->valSlots[i];
        }
        if (!right->isLeaf) {
            for (int i = 1; i <= right->count; ++i) {
                right->childPtrs[i - 1] = right->childPtrs[i];
            }
        }
        ++child->count;
        --right->count;
    }

    void mergeChildren(int idx) {
        BNode *left = childPtrs[idx];
        BNode *right = childPtrs[idx + 1];

        left->keySlots[T - 1] = keySlots[idx];
        left->valSlots[T - 1] = valSlots[idx];

        for (int i = 0; i < right->count; ++i) {
            left->keySlots[i + T] = right->keySlots[i];
            left->valSlots[i + T] = right->valSlots[i];
        }
        if (!left->isLeaf) {
            for (int i = 0; i <= right->count; ++i) {
                left->childPtrs[i + T] = right->childPtrs[i];
            }
        }

        for (int i = idx + 1; i < count; ++i) {
            keySlots[i - 1] = keySlots[i];
            valSlots[i - 1] = valSlots[i];
        }
        for (int i = idx + 2; i <= count; ++i) {
            childPtrs[i - 1] = childPtrs[i];
        }

        left->count += right->count + 1;
        --count;

        right->isLeaf = true;
        for (int i = 0; i <= right->count; ++i) {
            right->childPtrs[i] = nullptr;
        }
        delete right;
    }

    bool dump(ofstream &fo) {
        if (!writeHeaderAndEntries(fo)) {
            return false;
        }
        if (!isLeaf) {
            for (int i = 0; i <= count; ++i) {
                if (!childPtrs[i]->dump(fo)) {
                    return false;
                }
            }
        }
        return true;
    }

    static BNode *load(ifstream &fi) {
        bool leafFlag;
        if (!readLeafMarker(fi, leafFlag)) {
            return nullptr;
        }
        BNode *node = new BNode(leafFlag);
        readNodeBody(fi, node);
        if (!leafFlag) {
            readSubtrees(fi, node, leafFlag);
        }
        return node;
    }

   private:
    int findPosition(const string &key) const {
        return static_cast<int>(lower_bound(keySlots, keySlots + count, key) - keySlots);
    }

    bool isMatchAt(int pos, const string &key) const {
        return (pos < count && keySlots[pos] == key);
    }

    bool isInvalidChild(int pos) const {
        return (pos > count || childPtrs[pos] == nullptr);
    }

    pair<bool, uint64_t> notFound() const { return {false, 0}; }

    pair<bool, uint64_t> descendSearch(int pos, const string &key) {
        return childPtrs[pos]->search(key);
    }

    void insertIntoLeafAtSorted(const string &key, uint64_t value) {
        int idx = count - 1;
        while (idx >= 0 && keySlots[idx] > key) {
            keySlots[idx + 1] = keySlots[idx];
            valSlots[idx + 1] = valSlots[idx];
            --idx;
        }
        keySlots[idx + 1] = key;
        valSlots[idx + 1] = value;
        ++count;
    }

    int findInsertChildIndex(const string &key) const {
        int idx = count - 1;
        while (idx >= 0 && keySlots[idx] > key) {
            --idx;
        }
        return idx + 1;
    }

    void ensureChildWritable(int &idx, const string &key) {
        if (childPtrs[idx]->count == 2 * T - 1) {
            splitChild(idx);
            if (keySlots[idx] < key) {
                ++idx;
            }
        }
    }

    BNode *allocateSibling(BNode *origin) {
        BNode *sibling = new BNode(origin->isLeaf);
        sibling->count = T - 1;
        return sibling;
    }

    void moveKeysToSibling(BNode *origin, BNode *sibling) {
        for (int i = 0; i < T - 1; ++i) {
            sibling->keySlots[i] = origin->keySlots[i + T];
            sibling->valSlots[i] = origin->valSlots[i + T];
        }
    }

    void moveChildrenToSibling(BNode *origin, BNode *sibling) {
        if (!origin->isLeaf) {
            for (int i = 0; i < T; ++i) {
                sibling->childPtrs[i] = origin->childPtrs[i + T];
            }
        }
    }

    void insertSiblingIntoParent(int index, BNode *sibling) {
        for (int i = count; i >= index + 1; --i) {
            childPtrs[i + 1] = childPtrs[i];
        }
        childPtrs[index + 1] = sibling;
    }

    void liftMedianFromOrigin(int index, BNode *origin) {
        for (int i = count - 1; i >= index; --i) {
            keySlots[i + 1] = keySlots[i];
            valSlots[i + 1] = valSlots[i];
        }
        keySlots[index] = origin->keySlots[T - 1];
        valSlots[index] = origin->valSlots[T - 1];
        ++count;
    }

    bool removeWhenFound(int idx) {
        if (isLeaf) {
            eraseAtIndex(idx);
            return true;
        }
        return removeFromInternal(idx);
    }

    void prepareChildForRemoval(int &idx) {
        if (childPtrs[idx]->count < T) {
            rebalanceChild(idx);
        }
    }

    int adjustIndexAfterRebalance(int idx, bool wasRightBoundary) const {
        if (wasRightBoundary && idx > count) {
            return idx - 1;
        }
        return idx;
    }

    bool writeHeaderAndEntries(ofstream &fo) {
        fo.write(reinterpret_cast<char *>(&isLeaf), sizeof(isLeaf));
        fo.write(reinterpret_cast<char *>(&count), sizeof(count));
        for (int i = 0; i < count; ++i) {
            size_t slen = keySlots[i].size();
            fo.write(reinterpret_cast<char *>(&slen), sizeof(slen));
            fo.write(keySlots[i].data(), slen);
            fo.write(reinterpret_cast<char *>(&valSlots[i]), sizeof(valSlots[i]));
        }
        return true;
    }

    static bool readLeafMarker(ifstream &fi, bool &leafFlag) {
        return static_cast<bool>(fi.read(reinterpret_cast<char *>(&leafFlag), sizeof(bool)));
    }

    static void readNodeBody(ifstream &fi, BNode *node) {
        fi.read(reinterpret_cast<char *>(&node->count), sizeof(node->count));
        for (int i = 0; i < node->count; ++i) {
            size_t len = 0;
            fi.read(reinterpret_cast<char *>(&len), sizeof(len));
            node->keySlots[i].resize(len);
            fi.read(&node->keySlots[i][0], len);
            fi.read(reinterpret_cast<char *>(&node->valSlots[i]), sizeof(node->valSlots[i]));
        }
    }

    static void readSubtrees(ifstream &fi, BNode *node, bool leafFlag) {
        (void)leafFlag;
        for (int i = 0; i <= node->count; ++i) {
            node->childPtrs[i] = load(fi);
        }
    }
};

class BTree {
   public:
    BNode *root;
    BTree() : root(new BNode(true)) {}
    ~BTree() { delete root; }

    bool add(const string &word, uint64_t val) {
        if (root->search(word).first) {
            return false;
        }

        if (root->count == 2 * T - 1) {
            BNode *newRoot = new BNode(false);
            newRoot->childPtrs[0] = root;
            newRoot->splitChild(0);

            int childIndex = (newRoot->keySlots[0] < word) ? 1 : 0;
            newRoot->childPtrs[childIndex]->insertIntoNonFull(word, val);

            root = newRoot;
        } else {
            root->insertIntoNonFull(word, val);
        }
        return true;
    }

    bool remove(const string &v) {
        if (!root->count) {
            return false;
        }
        bool found = root->remove(v);
        if (root->count == 0 && !root->isLeaf) {
            BNode *oldRoot = root;
            root = root->childPtrs[0];
            oldRoot->childPtrs[0] = nullptr;
            delete oldRoot;
        }
        return found;
    }

    pair<bool, uint64_t> search(const string &word) {
        if (!root) {
            return {false, 0};
        }
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
        if (line.empty()) {
            continue;
        }

        const char op = line[0];
        if (op == '+') {
            istringstream iss(line);
            string cmd, word;
            uint64_t val = 0;
            iss >> cmd >> word >> val;
            word = lowercase_copy(word);
            cout << (tree.add(word, val) ? "OK" : "Exist") << '\n';
        } else if (op == '-') {
            string word = lowercase_copy(line.substr(2));
            cout << (tree.remove(word) ? "OK" : "NoSuchWord") << '\n';
        } else if (op == '!') {
            istringstream iss(line);
            string bang, cmd, path;
            iss >> bang >> cmd >> path;
            string err;
            if (cmd == "Save") {
                cout << (tree.dump(path, err) ? "OK" : "ERROR: " + err) << '\n';
            } else if (cmd == "Load") {
                cout << (tree.load(path, err) ? "OK" : "ERROR: " + err) << '\n';
            }
        } else {
            string word = lowercase_copy(line);
            auto res = tree.search(word);
            if (res.first) {
                cout << "OK: " << res.second << '\n';
            } else {
                cout << "NoSuchWord\n";
            }
        }
    }
    return 0;
}