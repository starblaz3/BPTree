#include "RecordPtr.hpp"
#include "LeafNode.hpp"

LeafNode::LeafNode(const TreePtr &tree_ptr) : TreeNode(LEAF, tree_ptr) {
    this->data_pointers.clear();
    this->next_leaf_ptr = NULL_PTR;
    if(!is_null(tree_ptr))
        this->load();
}

//returns max key within this leaf
Key LeafNode::max() {
    auto it = this->data_pointers.rbegin();
    return it->first;
}

// Merge the given node into our current node
Key LeafNode::merge_node_data(TreePtr tree_ptr, Key parent_key) {
    LeafNode other_node = LeafNode(tree_ptr);
    bool from_left = this->max() > other_node.max();
    this->size += other_node.size;
    for (auto it : other_node.data_pointers)
        this->data_pointers.insert({it.first, it.second});
    
    if (!from_left) this->next_leaf_ptr = other_node.next_leaf_ptr;
    this->dump();
    return this->max();
}

// Redistributes the given node data into our current node
Key LeafNode::redistribute_node_data(TreePtr tree_ptr, Key parent_key) {
    auto tree_node = LeafNode(tree_ptr);
    bool from_left = this->max() > tree_node.max();

    map<Key, RecordPtr> new_data_pointers;
    new_data_pointers.insert(this->data_pointers.begin(),this->data_pointers.end());
    new_data_pointers.insert(tree_node.data_pointers.begin(),tree_node.data_pointers.end());

    this->data_pointers.clear();
    tree_node.data_pointers.clear();

    int cur_pos = 0;
    for (auto it : new_data_pointers) {
        if ((cur_pos < MIN_OCCUPANCY && from_left) || (cur_pos >= MIN_OCCUPANCY && !from_left))
            tree_node.data_pointers.insert({it.first, it.second});
        else this->data_pointers.insert({it.first, it.second});
        cur_pos++;
    }

    this->size = this->data_pointers.size();
    tree_node.size = tree_node.data_pointers.size();

    this->dump();
    tree_node.dump();

    return (from_left? tree_node.max() : this->max());
}

//inserts <key, record_ptr> to leaf. If overflow occurs, leaf is split
//split node is returned
TreePtr LeafNode::insert_key(const Key &key, const RecordPtr &record_ptr) {
    TreePtr new_leaf = NULL_PTR; //if leaf is split, new_leaf = ptr to new split node ptr

    this->data_pointers.insert({key, record_ptr});
    this->size++;

    if (this->overflows()) {
        // Do something to split this shit
        auto split_node = new LeafNode(NULL_PTR);
        split_node->size = 0;
        split_node->node_type = LEAF;
        split_node->next_leaf_ptr = this->next_leaf_ptr;

        int el_ind = 0;
        vector<Key> keysToBeRemoved;
        for (auto it : this->data_pointers) {
            if (el_ind >= MIN_OCCUPANCY) {
                keysToBeRemoved.push_back(it.first);
                split_node->data_pointers.insert({it.first, it.second});
                split_node->size++;
            }
            el_ind++;
        }

        for (auto key : keysToBeRemoved) this->data_pointers.erase(key);

        new_leaf = split_node->tree_ptr;
        this->next_leaf_ptr = new_leaf;
        this->size = MIN_OCCUPANCY;

        split_node->dump();
    }

    this->dump();
    return new_leaf;
}

//key is deleted from leaf if exists
void LeafNode::delete_key(const Key &key) {
    if (this->data_pointers.count(key)) {
        this->data_pointers.erase(key);
        this->size--;
    }
    this->dump();
}

//runs range query on leaf
void LeafNode::range(ostream &os, const Key &min_key, const Key &max_key) const {
    BLOCK_ACCESSES++;
    for(const auto& data_pointer : this->data_pointers){
        if(data_pointer.first >= min_key && data_pointer.first <= max_key)
            data_pointer.second.write_data(os);
        if(data_pointer.first > max_key)
            return;
    }
    if(!is_null(this->next_leaf_ptr)){
        auto next_leaf_node = new LeafNode(this->next_leaf_ptr);
        next_leaf_node->range(os, min_key, max_key);
        delete next_leaf_node;
    }
}

//exports node - used for grading
void LeafNode::export_node(ostream &os) {
    TreeNode::export_node(os);
    for(const auto& data_pointer : this->data_pointers){
        os << data_pointer.first << " ";
    }
    os << endl;
}

//writes leaf as a mermaid chart
void LeafNode::chart(ostream &os) {
    string chart_node = this->tree_ptr + "[" + this->tree_ptr + BREAK;
    chart_node += "size: " + to_string(this->size) + BREAK;
    for(const auto& data_pointer: this->data_pointers) {
        chart_node += to_string(data_pointer.first) + " ";
    }
    chart_node += "]";
    os << chart_node << endl;
}

ostream& LeafNode::write(ostream &os) const {
    TreeNode::write(os);
    for(const auto & data_pointer : this->data_pointers){
        if(&os == &cout)
            os << "\n" << data_pointer.first << ": ";
        else
            os << "\n" << data_pointer.first << " ";
        os << data_pointer.second;
    }
    os << endl;
    os << this->next_leaf_ptr << endl;
    return os;
}

istream& LeafNode::read(istream& is){
    TreeNode::read(is);
    this->data_pointers.clear();
    for(int i = 0; i < this->size; i++){
        Key key = DELETE_MARKER;
        RecordPtr record_ptr;
        if(&is == &cin)
            cout << "K: ";
        is >> key;
        if(&is == &cin)
            cout << "P: ";
        is >> record_ptr;
        this->data_pointers.insert(pair<Key,RecordPtr>(key, record_ptr));
    }
    is >> this->next_leaf_ptr;
    return is;
}