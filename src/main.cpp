#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

const int DEGREE_LIMIT = 64;

string normalize_text_case(const string &input_text) {
    string normalized = input_text;
    for (size_t idx = 0; idx < normalized.length(); ++idx) 
        normalized[idx] = tolower(normalized[idx]);
    return normalized;
}

class KeyValueStorage {
   public:
    class TreeNode {
       public:
        string entries[2 * DEGREE_LIMIT - 1];
        uint64_t data[2 * DEGREE_LIMIT - 1];
        TreeNode *childNodes[2 * DEGREE_LIMIT];
        bool isLeafNode;
        int elementCount;

        TreeNode(bool isLeafNode_) {
            isLeafNode = isLeafNode_;
            elementCount = 0;
            for (int i = 0; i < 2 * DEGREE_LIMIT; i++) {
                childNodes[i] = nullptr;
            }
        }

        ~TreeNode() {
            if (!isLeafNode) {
                for (int j = 0; j <= elementCount; ++j) {
                    if (childNodes[j] != nullptr) {
                        delete childNodes[j];
                        childNodes[j] = nullptr;
                    }
                }
            }
        }

        pair<bool, uint64_t> lookup(const string &k) {
            int l = 0, r = elementCount - 1;
            while (r >= l) {
                int m = l + ((r - l) >> 1);
                if (entries[m] == k)
                    return {true, data[m]};
                else if (entries[m] < k)
                    l = m + 1;
                else
                    r = m - 1;
            }
            if (isLeafNode)
                return {false, 0};
            if (l > elementCount || childNodes[l] == nullptr)
                return {false, 0};
            return childNodes[l]->lookup(k);
        }

        void insertIntoLeaf(const string &k, uint64_t v) {
            int idx = elementCount - 1;
            while (idx >= 0 && entries[idx] > k) {
                entries[idx + 1] = entries[idx];
                data[idx + 1] = data[idx];
                --idx;
            }
            entries[idx + 1] = k;
            data[idx + 1] = v;
            elementCount++;
        }

        int findChildIndex(const string &k) {
            int idx = elementCount - 1;
            while (idx >= 0 && entries[idx] > k)
                idx--;
            return idx + 1;
        }

        void insertIntoInternalNode(const string &k, uint64_t v) {
            int idx = findChildIndex(k);
            if (childNodes[idx]->elementCount == 2 * DEGREE_LIMIT - 1) {
                divideNode(idx);
                if (entries[idx] < k)
                    idx++;
            }
            childNodes[idx]->addElement(k, v);
        }

        void addElement(const string &k, uint64_t v) {
            if (isLeafNode) {
                insertIntoLeaf(k, v);
            } else {
                insertIntoInternalNode(k, v);
            }
        }

        void moveHalfToNewNode(TreeNode *origin, TreeNode *newSibling) {
            for (int i = 0; i < DEGREE_LIMIT - 1; ++i) {
                newSibling->entries[i] = origin->entries[i + DEGREE_LIMIT];
                newSibling->data[i] = origin->data[i + DEGREE_LIMIT];
            }
            if (!origin->isLeafNode) {
                for (int i = 0; i < DEGREE_LIMIT; ++i)
                    newSibling->childNodes[i] = origin->childNodes[i + DEGREE_LIMIT];
            }
        }

        void shiftChildPointers(int index) {
            for (int i = elementCount; i >= index + 1; --i)
                childNodes[i + 1] = childNodes[i];
        }

        void shiftKeysAndData(int index) {
            for (int i = elementCount - 1; i >= index; --i) {
                entries[i + 1] = entries[i];
                data[i + 1] = data[i];
            }
        }

        void divideNode(int index) {
            TreeNode *origin = childNodes[index];
            TreeNode *newSibling = new TreeNode(origin->isLeafNode);
            newSibling->elementCount = DEGREE_LIMIT - 1;

            moveHalfToNewNode(origin, newSibling);
            origin->elementCount = DEGREE_LIMIT - 1;

            shiftChildPointers(index);
            childNodes[index + 1] = newSibling;

            shiftKeysAndData(index);
            entries[index] = origin->entries[DEGREE_LIMIT - 1];
            data[index] = origin->data[DEGREE_LIMIT - 1];
            elementCount++;
        }

        bool replaceWithPredecessorOrSuccessor(int idx) {
            string delKey = entries[idx];
            if (childNodes[idx]->elementCount >= DEGREE_LIMIT) {
                auto pred = findPredecessor(idx);
                entries[idx] = pred.first;
                data[idx] = pred.second;
                return childNodes[idx]->deleteElement(pred.first);
            } else if (childNodes[idx + 1]->elementCount >= DEGREE_LIMIT) {
                auto succ = findSuccessor(idx);
                entries[idx] = succ.first;
                data[idx] = succ.second;
                return childNodes[idx + 1]->deleteElement(succ.first);
            } else {
                joinNodes(idx);
                return childNodes[idx]->deleteElement(delKey);
            }
        }

        int findKeyIndex(const string &key) {
            int idx = 0;
            while (idx < elementCount && entries[idx] < key)
                idx++;
            return idx;
        }

        bool deleteFromLeafNode(const string &key, int idx) {
            if (idx < elementCount && entries[idx] == key) {
                removeFromLeaf(idx);
                return true;
            }
            return false;
        }

        bool deleteFromInternalNode(const string &key, int idx) {
            if (idx < elementCount && entries[idx] == key) {
                return replaceWithPredecessorOrSuccessor(idx);
            } else {
                bool at_end = (idx == elementCount);
                if (childNodes[idx]->elementCount < DEGREE_LIMIT)
                    ensureMinimumKeys(idx);
                if (at_end && idx > elementCount)
                    idx--;
                return childNodes[idx]->deleteElement(key);
            }
        }

        bool deleteElement(const string &key) {
            int idx = findKeyIndex(key);
            
            if (isLeafNode) {
                return deleteFromLeafNode(key, idx);
            } else {
                return deleteFromInternalNode(key, idx);
            }
        }

        void removeFromLeaf(int idx) {
            for (int j = idx + 1; j < elementCount; ++j) {
                entries[j - 1] = entries[j];
                data[j - 1] = data[j];
            }
            elementCount--;
        }

        pair<string, uint64_t> findPredecessor(int t) {
            TreeNode *cur = childNodes[t];
            while (!cur->isLeafNode)
                cur = cur->childNodes[cur->elementCount];
            return {cur->entries[cur->elementCount - 1], cur->data[cur->elementCount - 1]};
        }

        pair<string, uint64_t> findSuccessor(int t) {
            TreeNode *cur = childNodes[t + 1];
            while (!cur->isLeafNode)
                cur = cur->childNodes[0];
            return {cur->entries[0], cur->data[0]};
        }

        void ensureMinimumKeys(int idx) {
            if (idx != 0 && childNodes[idx - 1]->elementCount >= DEGREE_LIMIT)
                borrowFromLeftSibling(idx);
            else if (idx != elementCount && childNodes[idx + 1]->elementCount >= DEGREE_LIMIT)
                borrowFromRightSibling(idx);
            else {
                if (idx != elementCount)
                    joinNodes(idx);
                else
                    joinNodes(idx - 1);
            }
        }

        void borrowFromLeftSibling(int idx) {
            TreeNode *curChild = childNodes[idx];
            TreeNode *leftSibling = childNodes[idx - 1];

            for (int i = curChild->elementCount - 1; i >= 0; --i) {
                curChild->entries[i + 1] = curChild->entries[i];
                curChild->data[i + 1] = curChild->data[i];
            }

            if (!curChild->isLeafNode) {
                for (int i = curChild->elementCount; i >= 0; --i) {
                    curChild->childNodes[i + 1] = curChild->childNodes[i];
                }
            }

            curChild->entries[0] = entries[idx - 1];
            curChild->data[0] = data[idx - 1];

            if (!curChild->isLeafNode) {
                curChild->childNodes[0] = leftSibling->childNodes[leftSibling->elementCount];
            }

            entries[idx - 1] = leftSibling->entries[leftSibling->elementCount - 1];
            data[idx - 1] = leftSibling->data[leftSibling->elementCount - 1];

            curChild->elementCount++;
            leftSibling->elementCount--;
        }

        void borrowFromRightSibling(int idx) {
            TreeNode *child = childNodes[idx];
            TreeNode *rightSibling = childNodes[idx + 1];

            child->entries[child->elementCount] = entries[idx];
            child->data[child->elementCount] = data[idx];

            if (!child->isLeafNode) {
                child->childNodes[child->elementCount + 1] = rightSibling->childNodes[0];
            }

            entries[idx] = rightSibling->entries[0];
            data[idx] = rightSibling->data[0];

            for (int i = 1; i < rightSibling->elementCount; ++i) {
                rightSibling->entries[i - 1] = rightSibling->entries[i];
                rightSibling->data[i - 1] = rightSibling->data[i];
            }

            if (!rightSibling->isLeafNode) {
                for (int i = 1; i <= rightSibling->elementCount; ++i) {
                    rightSibling->childNodes[i - 1] = rightSibling->childNodes[i];
                }
            }

            child->elementCount++;
            rightSibling->elementCount--;
        }

        void mergeKeysFromRight(TreeNode *leftChild, TreeNode *rightChild, int idx) {
            leftChild->entries[DEGREE_LIMIT - 1] = entries[idx];
            leftChild->data[DEGREE_LIMIT - 1] = data[idx];

            for (int i = 0; i < rightChild->elementCount; ++i) {
                leftChild->entries[i + DEGREE_LIMIT] = rightChild->entries[i];
                leftChild->data[i + DEGREE_LIMIT] = rightChild->data[i];
            }
        }

        void mergeChildPointersFromRight(TreeNode *leftChild, TreeNode *rightChild) {
            if (!leftChild->isLeafNode) {
                for (int i = 0; i <= rightChild->elementCount; ++i)
                    leftChild->childNodes[i + DEGREE_LIMIT] = rightChild->childNodes[i];
            }
        }

        void removeKeyFromParent(int idx) {
            for (int i = idx + 1; i < elementCount; ++i) {
                entries[i - 1] = entries[i];
                data[i - 1] = data[i];
            }
            for (int i = idx + 2; i <= elementCount; ++i)
                childNodes[i - 1] = childNodes[i];
            elementCount--;
        }

        void cleanupRightChild(TreeNode *rightChild) {
            rightChild->isLeafNode = true;
            for (int i = 0; i <= rightChild->elementCount; ++i)
                rightChild->childNodes[i] = nullptr;
            delete rightChild;
        }

        void joinNodes(int idx) {
            TreeNode *leftChild = childNodes[idx];
            TreeNode *rightChild = childNodes[idx + 1];

            mergeKeysFromRight(leftChild, rightChild, idx);
            mergeChildPointersFromRight(leftChild, rightChild);
            removeKeyFromParent(idx);
            
            leftChild->elementCount += rightChild->elementCount + 1;
            cleanupRightChild(rightChild);
        }

        bool serialize(ofstream &fo) {
            fo.write((char *)(&isLeafNode), sizeof(isLeafNode));
            fo.write((char *)(&elementCount), sizeof(elementCount));
            for (int i = 0; i < elementCount; ++i) {
                size_t slen = entries[i].size();
                fo.write((char *)(&slen), sizeof(slen));
                fo.write(entries[i].data(), slen);
                fo.write((char *)&data[i], sizeof(data[i]));
            }
            if (!isLeafNode) {
                for (int i = 0; i <= elementCount; ++i) {
                    if (!childNodes[i]->serialize(fo))
                        return false;
                }
            }
            return true;
        }

        static TreeNode *deserialize(ifstream &fi) {
            bool isLeafNode_status;
            if (!fi.read((char *)&isLeafNode_status, sizeof(bool)))
                return nullptr;

            TreeNode *node = new TreeNode(isLeafNode_status);

            if (!fi.read((char *)(&node->elementCount), sizeof(node->elementCount))) {
                delete node;
                return nullptr;
            }

            if (node->elementCount < 0 || node->elementCount > 2 * DEGREE_LIMIT - 1) {
                delete node;
                return nullptr;
            }

            for (int i = 0; i < node->elementCount; ++i) {
                size_t len = 0;
                if (!fi.read((char *)&len, sizeof(len))) {
                    delete node;
                    return nullptr;
                }
                if (len > 0) {
                    try {
                        node->entries[i].resize(len);
                    } catch (...) {
                        delete node;
                        return nullptr;
                    }
                    if (!fi.read(&node->entries[i][0], len)) {
                        delete node;
                        return nullptr;
                    }
                } else {
                    node->entries[i].clear();
                }
                if (!fi.read((char *)&node->data[i], sizeof(node->data[i]))) {
                    delete node;
                    return nullptr;
                }
            }
            if (!isLeafNode_status) {
                for (int i = 0; i <= node->elementCount; ++i) {
                    TreeNode *child = deserialize(fi);
                    if (!child) {
                        for (int j = 0; j < i; ++j) {
                            delete node->childNodes[j];
                            node->childNodes[j] = nullptr;
                        }
                        delete node;
                        return nullptr;
                    }
                    node->childNodes[i] = child;
                }
            }
            return node;
        }
    };

    TreeNode *root;
    KeyValueStorage() : root(new TreeNode(true)) {}
    ~KeyValueStorage() { delete root; }

    bool insertKeyValue(const string &word, uint64_t val) {
        if (root->lookup(word).first)
            return false;

        if (root->elementCount == 2 * DEGREE_LIMIT - 1) {
            TreeNode *newRoot = new TreeNode(false);
            newRoot->childNodes[0] = root;
            newRoot->divideNode(0);

            int childIndex = (newRoot->entries[0] < word) ? 1 : 0;
            newRoot->childNodes[childIndex]->addElement(word, val);

            root = newRoot;
        } else {
            root->addElement(word, val);
        }
        return true;
    }

    bool deleteKey(const string &v) {
        if (!root->elementCount)
            return false;
        bool found = root->deleteElement(v);
        if (root->elementCount == 0 && !root->isLeafNode) {
            TreeNode *oldRoot = root;
            root = root->childNodes[0];
            oldRoot->childNodes[0] = nullptr;
            delete oldRoot;
        }
        return found;
    }

    pair<bool, uint64_t> findValue(const string &word) {
        if (!root)
            return {false, 0};
        return root->lookup(word);
    }

    bool saveToFile(const string &filename, string &errmsg) {
        ofstream outFile(filename, std::ios::binary);
        if (!root->serialize(outFile)) {
            errmsg = "Serialize error";
            return false;
        }
        return true;
    }

    bool loadFromFile(const string &fname, string &errmsg) {
        ifstream inFile(fname, std::ios::binary);
        inFile.seekg(0, std::ios::end);
        std::streampos fileSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);
        if (fileSize == 0) {
            TreeNode *emptyRoot = new TreeNode(true);
            delete root;
            root = emptyRoot;
            return true;
        }

        TreeNode *newRoot = TreeNode::deserialize(inFile);
        if (!newRoot) {
            errmsg = "Deserialize error";
            return false;
        }

        if (inFile.peek() != EOF) {
            delete newRoot;
            errmsg = "Deserialize error";
            return false;
        }

        delete root;
        root = newRoot;
        return true;
    }
};

void processInsertCommand(KeyValueStorage &storage, const string &commandLine) {
    istringstream parser(commandLine);
    string operation, term;
    uint64_t value;
    parser >> operation >> term >> value;
    term = normalize_text_case(term);
    cout << (storage.insertKeyValue(term, value) ? "OK" : "Exist") << '\n';
}

void processDeleteCommand(KeyValueStorage &storage, const string &commandLine) {
    string term = normalize_text_case(commandLine.substr(2));
    cout << (storage.deleteKey(term) ? "OK" : "NoSuchWord") << '\n';
}

void processFileCommand(KeyValueStorage &storage, const string &commandLine) {
    istringstream parser(commandLine);
    string prefix, operation, filePath;
    parser >> prefix >> operation >> filePath;
    string errorMessage;
    if (operation == "Save")
        cout << (storage.saveToFile(filePath, errorMessage) ? "OK" : "ERROR: " + errorMessage) << '\n';
    else if (operation == "Load")
        cout << (storage.loadFromFile(filePath, errorMessage) ? "OK" : "ERROR: " + errorMessage) << '\n';
}

void processSearchCommand(KeyValueStorage &storage, const string &commandLine) {
    string term = normalize_text_case(commandLine);
    auto result = storage.findValue(term);
    if (result.first)
        cout << "OK: " << result.second << '\n';
    else
        cout << "NoSuchWord\n";
}

void processCommand(KeyValueStorage &storage, const string &inputLine) {
    if (inputLine.empty())
        return;
    
    char commandType = inputLine[0];
    switch (commandType) {
        case '+':
            processInsertCommand(storage, inputLine);
            break;
        case '-':
            processDeleteCommand(storage, inputLine);
            break;
        case '!':
            processFileCommand(storage, inputLine);
            break;
        default:
            processSearchCommand(storage, inputLine);
            break;
    }
}

int main() {
    std::ios::sync_with_stdio(false);
    cin.tie(nullptr);

    KeyValueStorage dataStorage;
    string inputLine;

    while (getline(cin, inputLine)) {
        processCommand(dataStorage, inputLine);
    }
    return 0;
}