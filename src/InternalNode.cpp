#include "InternalNode.hpp"

//creates internal node pointed to by tree_ptr or creates a new one
InternalNode::InternalNode(const TreePtr &tree_ptr) : TreeNode(INTERNAL, tree_ptr) {
    this->keys.clear();
    this->tree_pointers.clear();
    if (!is_null(tree_ptr))
        this->load();
}

//max element from tree rooted at this node
Key InternalNode::max() {
    Key max_key = DELETE_MARKER;
    TreeNode* last_tree_node = TreeNode::tree_node_factory(this->tree_pointers[this->size - 1]);
    max_key = last_tree_node->max();
    delete last_tree_node;
    return max_key;
}

//if internal node contains a single child, it is returned
TreePtr InternalNode::single_child_ptr() {
    if (this->size == 1)
        return this->tree_pointers[0];
    return NULL_PTR;
}

// Merge the given node into our current node
Key InternalNode::merge_node_data(TreePtr tree_ptr, Key parent_key) {
    InternalNode other_node = InternalNode(tree_ptr);
    bool from_left = this->max() > other_node.max();

    Key ret_val = 0;

    if (from_left) {
        this->keys.insert(this->keys.begin(), parent_key);
        this->keys.insert(this->keys.begin(), other_node.keys.begin(), other_node.keys.end());
        this->tree_pointers.insert(this->tree_pointers.begin(), other_node.tree_pointers.begin(), other_node.tree_pointers.end());
    } else {
        this->keys.push_back(parent_key);
        this->keys.insert(this->keys.end(), other_node.keys.begin(), other_node.keys.end());
        this->tree_pointers.insert(this->tree_pointers.end(), other_node.tree_pointers.begin(), other_node.tree_pointers.end());
        ret_val = other_node.keys[0];
    }

    this->size += other_node.size;
    if (from_left) ret_val = this->max();

    this->dump();
    return ret_val;
}

// Redistributes the given node data into our current node
Key InternalNode::redistribute_node_data(TreePtr tree_ptr, Key parent_key) {
    auto tree_node = InternalNode(tree_ptr);
    bool from_left = this->max() > tree_node.max();

    Key ret_val = 0;

    if (from_left) {
        this->keys.insert(this->keys.begin(), parent_key);
        this->tree_pointers.insert(this->tree_pointers.begin(), tree_node.tree_pointers.back());
        tree_node.tree_pointers.pop_back();
        tree_node.keys.pop_back();
    } else {
        this->keys.push_back(parent_key);
        this->tree_pointers.push_back(tree_node.tree_pointers[0]);
        ret_val = tree_node.keys[0];
        tree_node.tree_pointers.erase(tree_node.tree_pointers.begin());
        tree_node.keys.erase(tree_node.keys.begin());
    }

    this->size = this->tree_pointers.size();
    tree_node.size = tree_node.tree_pointers.size();

    if (from_left) ret_val = tree_node.max();

    this->dump();
    tree_node.dump();

    return ret_val;
}

//inserts <key, record_ptr> into subtree rooted at this node.
//returns pointer to split node if exists
TreePtr InternalNode::insert_key(const Key &key, const RecordPtr &record_ptr) {
    TreePtr new_tree_ptr = NULL_PTR;
    
    int i = min((int) (lower_bound(this->keys.begin(), this->keys.end(), key) - keys.begin()), (int)this->keys.size());    
    auto tree_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
    auto split_node_ptr = tree_node->insert_key(key, record_ptr);
    delete tree_node;

    if (split_node_ptr != NULL_PTR) {
        this->tree_pointers.insert(this->tree_pointers.begin() + i + 1, split_node_ptr);
        
        auto split_node = TreeNode::tree_node_factory(split_node_ptr);
        this->keys.insert(this->keys.begin() + i + 1, split_node->max());
        delete split_node;

        auto left_out_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
        this->keys[i] = left_out_node->max();
        delete left_out_node;

        this->size++;
    }

    if (this->overflows()) {
        auto split_node = new InternalNode(NULL_PTR);
        split_node->size = 0;
        split_node->node_type = INTERNAL;

        for (int idx = 0; idx < this->size; idx++) {
            if (idx >= MIN_OCCUPANCY) {
                split_node->keys.push_back(this->keys[idx]);
                split_node->tree_pointers.push_back(this->tree_pointers[idx]);
                split_node->size++;
            }
        }

        this->keys.erase(this->keys.begin() + MIN_OCCUPANCY, this->keys.end());
        this->tree_pointers.erase(this->tree_pointers.begin() + MIN_OCCUPANCY, this->tree_pointers.end());

        new_tree_ptr = split_node->tree_ptr;
        this->size = MIN_OCCUPANCY;

        split_node->dump();
    }

    this->dump();
    return new_tree_ptr;
}

//deletes key from subtree rooted at this if exists
void InternalNode::delete_key(const Key &key) {
    int entry_pos = (lower_bound(this->keys.begin(), this->keys.end(), key) - keys.begin());
    int i = min(entry_pos, (int)this->keys.size());
    auto tree_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
    tree_node->delete_key(key);

    if (tree_node->underflows()) {
        bool left_sib_exists = ((i - 1) >= 0);
        bool right_sib_exists = ((i + 1) < this->size);

        auto left_sib_ptr = (left_sib_exists ? this->tree_pointers[i - 1] : NULL_PTR);
        auto right_sib_ptr = (right_sib_exists ? this->tree_pointers[i + 1] : NULL_PTR);

        auto left_sib = TreeNode::tree_node_factory(left_sib_ptr);
        auto right_sib = TreeNode::tree_node_factory(right_sib_ptr);

        if (left_sib_exists && left_sib->size > MIN_OCCUPANCY) {
            // REDISTRIBUTE from left
            this->keys[i - 1] = tree_node->redistribute_node_data(left_sib_ptr, this->keys[i - 1]);
        } else if (left_sib_exists && left_sib->size <= MIN_OCCUPANCY) {
            // Take left node and push into me
            this->keys[i] = tree_node->merge_node_data(left_sib_ptr, this->keys[i - 1]);
            this->tree_pointers.erase(this->tree_pointers.begin() + i - 1);
            this->keys.erase(this->keys.begin() + i - 1);
            remove((TEMP_PATH + left_sib_ptr).c_str());
            this->size--;
        } else if (right_sib_exists && right_sib->size > MIN_OCCUPANCY) {
            // REDISTRIBUTE from right
            this->keys[i] = tree_node->redistribute_node_data(right_sib_ptr, this->keys[i]);
        } else if (right_sib_exists && right_sib->size <= MIN_OCCUPANCY) {
            // Take right node and push into me
            this->keys[i] = tree_node->merge_node_data(right_sib_ptr, this->keys[i]);
            this->tree_pointers.erase(this->tree_pointers.begin() + i + 1);
            this->keys.erase(this->keys.begin() + i);
            remove((TEMP_PATH + right_sib_ptr).c_str());
            this->size--;
        } else {
            cout << "J: Unable to work out any of these - shouldnt be possible.\nH: Jesse, what the **** are you talking about.";
        }
    }

    this->dump();
}

//runs range query on subtree rooted at this node
void InternalNode::range(ostream &os, const Key &min_key, const Key &max_key) const {
    BLOCK_ACCESSES++;
    for (int i = 0; i < this->size - 1; i++) {
        if (min_key <= this->keys[i]) {
            auto* child_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
            child_node->range(os, min_key, max_key);
            delete child_node;
            return;
        }
    }
    auto* child_node = TreeNode::tree_node_factory(this->tree_pointers[this->size - 1]);
    child_node->range(os, min_key, max_key);
    delete child_node;
}

//exports node - used for grading
void InternalNode::export_node(ostream &os) {
    TreeNode::export_node(os);
    for (int i = 0; i < this->size - 1; i++)
        os << this->keys[i] << " ";
    os << endl;
    for (int i = 0; i < this->size; i++) {
        auto child_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
        child_node->export_node(os);
        delete child_node;
    }
}

//writes subtree rooted at this node as a mermaid chart
void InternalNode::chart(ostream &os) {
    string chart_node = this->tree_ptr + "[" + this->tree_ptr + BREAK;
    chart_node += "size: " + to_string(this->size) + BREAK;
    chart_node += "]";
    os << chart_node << endl;

    for (int i = 0; i < this->size; i++) {
        auto tree_node = TreeNode::tree_node_factory(this->tree_pointers[i]);
        tree_node->chart(os);
        delete tree_node;
        string link = this->tree_ptr + "-->|";

        if (i == 0)
            link += "x <= " + to_string(this->keys[i]);
        else if (i == this->size - 1) {
            link += to_string(this->keys[i - 1]) + " < x";
        } else {
            link += to_string(this->keys[i - 1]) + " < x <= " + to_string(this->keys[i]);
        }
        link += "|" + this->tree_pointers[i];
        os << link << endl;
    }
}

ostream& InternalNode::write(ostream &os) const {
    TreeNode::write(os);
    for (int i = 0; i < this->size - 1; i++) {
        if (&os == &cout)
            os << "\nP" << i + 1 << ": ";
        os << this->tree_pointers[i] << " ";
        if (&os == &cout)
            os << "\nK" << i + 1 << ": ";
        os << this->keys[i] << " ";
    }
    if (&os == &cout)
        os << "\nP" << this->size << ": ";
    os << this->tree_pointers[this->size - 1];
    return os;
}

istream& InternalNode::read(istream& is) {
    TreeNode::read(is);
    this->keys.assign(this->size - 1, DELETE_MARKER);
    this->tree_pointers.assign(this->size, NULL_PTR);
    for (int i = 0; i < this->size - 1; i++) {
        if (&is == &cin)
            cout << "P" << i + 1 << ": ";
        is >> this->tree_pointers[i];
        if (&is == &cin)
            cout << "K" << i + 1 << ": ";
        is >> this->keys[i];
    }
    if (&is == &cin)
        cout << "P" << this->size;
    is >> this->tree_pointers[this->size - 1];
    return is;
}
