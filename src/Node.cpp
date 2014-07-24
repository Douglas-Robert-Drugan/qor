#include "Node.h"
#include "Common.h"
#include "Filesystem.h"
using namespace std;

Node :: Node(const std::string& fn):
    m_Filename(fn),
    m_pConfig(std::make_shared<Meta>())
{
    init();
    if(Filesystem::getExtension(fn)=="json")
    {
        try {
            m_pConfig = make_shared<Meta>(fn);
        } catch(const Error& e) {}
    }
}

Node :: Node(const std::string& fn, IFactory* factory, ICache* cache):
    Node(fn)
{
    init();
}

void Node :: init()
{
    m_WorldTransform = [this]() -> glm::mat4 {
        if(parent_c())
            return *parent_c()->matrix_c(Space::WORLD) * *matrix_c();
        else
            return *matrix_c();
    };
    m_WorldBox = std::bind(&Node::calculate_world_box, this);
}

void Node :: parents(std::queue<const Node*>& q, bool include_self) const
{
    const Node* parent = m_pParent;
    if(include_self)
        q.push(this);
    while(parent)
    {
        q.push(parent);
        parent = parent->parent_c();
    }
}

void Node :: parents(std::stack<const Node*>& s, bool include_self) const
{
    const Node* parent = m_pParent;
    if(include_self)
        s.push(this);
    while(parent)
    {
        s.push(parent);
        parent = parent->parent_c();
    }
}

void Node :: parents(std::queue<Node*>& q, bool include_self)
{
    Node* parent = m_pParent;
    if(include_self)
        q.push(this);
    while(parent)
    {
        q.push(parent);
        parent = parent->parent();
    }
}

void Node :: parents(std::stack<Node*>& s, bool include_self)
{
    Node* parent = m_pParent;
    if(include_self)
        s.push(this);
    while(parent)
    {
        s.push(parent);
        parent = parent->parent();
    }
}

const glm::mat4* Node :: matrix_c(Space s) const
{
    assert(s != Space::LOCAL); // this would be identity

    if(s == Space::PARENT)
        return matrix_c();

    return &m_WorldTransform();
}


glm::vec3 Node :: position(Space s) const
{
    assert(s != Space::LOCAL);
    if(s == Space::PARENT)
        return Matrix::translation(m_Transform);
    else if (s == Space::WORLD)
        return Matrix::translation(*matrix_c(Space::WORLD));
    assert(false);
    return glm::vec3();
}

void Node :: position(const glm::vec3& v, Space s)
{
    assert(s != Space::LOCAL); // didn't you mean parent?
    assert(s != Space::WORLD);
    Matrix::translation(m_Transform, v);
    pend();
}

void Node :: move(const glm::vec3& v, Space s)
{
    assert(s != Space::WORLD); // didn't you mean parent?
    //if(s ==Space::WORLD)
    //    Matrix::translate(m_Transform, transform_out(v));
    //else
        //Matrix::translate(m_Transform, v);

    if(s == Space::LOCAL)
        Matrix::translate(m_Transform, Matrix::orientation(m_Transform) * v);
    else
        Matrix::translate(m_Transform, v);
    pend();
}

void Node :: scale(glm::vec3 f)
{
    //assert(s != Space::WORLD);
    Matrix::scale(m_Transform, f);
    pend();
}

void Node :: rescale(glm::vec3 f)
{
    //assert(s != Space::WORLD);
    Matrix::rescale(m_Transform, f);
    pend();
}

void Node :: scale(float f)
{
    Matrix::scale(m_Transform, f);
    pend();
}

void Node :: rescale(float f)
{
    Matrix::rescale(m_Transform, f);
    pend();
}

void Node :: rotate(float tau, const glm::vec3& v, Space s)
{
    assert(s != Space::WORLD);
    switch(s)
    {
        case Space::LOCAL:
            m_Transform *= glm::rotate(tau * 360.0f, v);
            break;
        case Space::PARENT:
            m_Transform = glm::rotate(tau * 360.0f, v) * m_Transform;
            break;
        default:
            break;
    }
    pend();
}

void Node :: render(Pass* pass) const
{
    // render self
    if(visible())
    {
        pass->matrix(matrix_c(Space::WORLD));
        render_self(pass);
    }

    // render children
    if(pass && pass->recursive())
        for(const auto& c: m_Children)
            c->render(pass);
}

void Node :: logic(Freq::Time t)
{
    Actuation::logic(t);
    logic_self(t);
     
    m_ChildrenCopy = m_Children;
    for(const auto& c: m_ChildrenCopy)
        c->logic(t);
    m_ChildrenCopy.clear();
}

//void Node :: render_from(const glm::mat4& view_matrix, IPartitioner* partitioner, unsigned int flags) const
//{
//    // Grab queue of parents connected this node
//    std::queue<const Node*> parents;
//    parents(parents);

//    // Multiply GL matrix by inverse of self matrix
//    //glm::mat4 m(glm::mat4::INVERSE, *matrix_c());
//    glm::mat4 m = glm::inverse(*matrix_c());
//    glMultMatrixf(glm::value_ptr(m));

//    // Now go through parents in order from this node (camera) to world space
//    const Node* parent = nullptr;
//    while(!parents.empty())
//    {
//        parent = parents.front();
//        parents.pop();

//        // multiply by inverse of each parent
//        //m.clear(glm::mat4::INVERSE, *parent->matrix_c());
//        m = glm::inverse(*parent->matrix_c());
//        glMultMatrixf(glm::value_ptr(m));
//    }

//    //if(partitioner)
//    //    partitioner->refreshFrustum();
//}

const Node* Node :: find_c(const Node* n) const
{
    assert(n);
    if(n->parent_c() == this)
        return n;

    for(auto itr = m_Children.cbegin();
        itr != m_Children.cend();
        ++itr)
    {
        const Node* temp = (*itr)->find_c(n);
        if(temp)
            return temp;
    }

    //foreach(const Node* child, m_Children)
    //    child->find_c(n);

    return nullptr;
}

Node* Node :: find(Node* n)
{
    assert(n);
    if(n->parent_c() == this)
        return n;

    for(auto itr = m_Children.begin();
        itr != m_Children.end();
        ++itr)
    {
        Node* temp = (*itr)->find(n);
        if(temp)
            return temp;
    }

    //foreach(Node* child, m_Children)
    //    child->find(n);
    return nullptr;
}

//Node* Node ::add(Node* n)
//{
//    std::shared_ptr<Node> np(n);
//    return add(np);
//}

Node* Node ::add(const std::shared_ptr<Node>& n)
{
    assert(n);
    assert(this != n.get()); // can't add to self
    assert(n->parent() == nullptr); // node we're trying to add has no existing parent

    n->_set_parent(this);
    m_Children.push_back(n);
    n->pend();
    return n.get();
}

bool Node :: remove(Node* n, unsigned int flags)
{
    //assert(flags & PRESERVE); // deprecated: use preserve() instead

    for(auto itr = m_Children.begin();
        itr != m_Children.end();
        ++itr)
    {
        if(itr->get() == n)
        {
            //if(!(flags & PRESERVE))
            //    (*itr)->remove_all();

            //_onRemove(itr->get());
            //Node* delete_me = itr->get();
            m_Children.erase(itr);

            //if(!(flags & PRESERVE))
            //    delete delete_me;

            return true;
        }
        else if(flags & SEARCH_SUBNODES)
        {
            // recursively search subnodes for node to remove
            if((*itr)->remove(n,flags))
                return true;
        }
    }

    return false;
}

//std::shared_ptr<Node> Node :: preserve(Node* n, unsigned int flags)
//{
//    std::shared_ptr<Node> preserved_node;

//    for(auto itr = m_Children.begin();
//        itr != m_Children.end();
//        ++itr)
//    {
//        if(itr->get() == n)
//        {
//            (*itr)->remove_all(); // remove children
//            //_onRemove(itr->get());
//            (*itr)->_set_parent(nullptr);
//            preserved_node = *itr;
//            m_Children.erase(itr);
//            return preserved_node;
//        }
//        else if(flags & SEARCH_SUBNODES)
//        {
//            // recursively search subnodes for node to remove
//            if((preserved_node = (*itr)->preserve(n)))
//                return preserved_node;
//        }
//    }

//    return preserved_node;
//}

//unsigned int Node :: remove(Node* n, unsigned int flags)
//{
//    // keep track of # of removed nodes
//    int removed = 0;

//    for(list<Node*>::iterator itr = m_Children.begin();
//        itr != m_Children.end();
//        )
//    {
//        // SEARCH_SUBNODES flag: search subnodes for node to remove
//        if(*itr != n)
//        {
//            if(flags & SEARCH_SUBNODES)
//                removed += (*itr)->remove(n,flags);
//        }
//        else
//        {
//            (*itr)->removeAll();
//            onRemove(*itr);
//            //(*itr)->removeReference();
//            itr = m_Children.erase(itr);
//            removed++;
//
//            continue; // avoid itr++
//        }

//        ++itr; // increment here to avoid erase() itr invalidation
//    }

//    return removed;
//}

void Node :: remove_all(unsigned int flags)
{
   //assert(! (flags & PRESERVE));

   for(auto itr = m_Children.begin();
       itr != m_Children.end();
       )
   {
       //_onRemove(itr->get());
       //Node* delete_me = itr->get();
       itr = m_Children.erase(itr);
       //delete delete_me;
   }

   m_Children.clear();
}

//void Node :: removeAll(list<Node*>& removed_nodes, unsigned int flags)
//{
//    assert(flags & PRESERVE);

//    for(auto itr = m_Children.begin();
//        itr != m_Children.end();
//        )
//    {
//        _onRemove(itr->get());
//        removed_nodes.push_back(itr->get()); // preserve (still contains subnodes!)
//        itr = m_Children.erase(itr);
//    }

//    m_Children.clear();
//}


//void Node :: removeAll(list<shared_ptr<Node>>& removed_nodes, unsigned int flags)
//{
//    assert(flags & PRESERVE);

//    for(auto itr = m_Children.begin();
//        itr != m_Children.end();
//        )
//    {
//        _onRemove(itr->get());
//        removed_nodes.push_back(itr); // preserve (still contains subnodes!)
//        itr = m_Children.erase(itr);
//    }

//    m_Children.clear();
//}


void Node :: detach()
{
    if(m_pParent) {
        m_pParent->remove(this);
        m_pParent = nullptr;
    }
}

void Node :: collapse(Space s, unsigned int flags)
{

    if(s == Space::PARENT)
    {
        if(!parent()) // if this node is root, exit
            WARNING("attempt to collapse root ndoe");
        if(!parent()->parent()) // if parent is root, exit
            WARNING("node already collapsed");

        // dettach self from parent
        //parent()->remove(this, PRESERVE);
        //std::shared_ptr<Node> preserve_me = parent()->preserve(this);
        // Combine transformations
        auto self(shared_from_this());
        m_Transform = *parent()->matrix() * m_Transform;
        auto old_parent = self->parent();
        self->detach();

        // reassign parent
        old_parent->parent()->add(self);

        //_setParent(parent()->parent()); // bad

        assert(m_pParent); // if this node becomes root, we've done something wrong
    }
    else if(s == Space::WORLD)
    {
        if(!parent())
            WARNING("node already collapsed");

        while(parent()->parent())
            collapse(Space::PARENT);
    }
    else {
        WARNING("Collapsing a node to local space has no effect.");
    }
}

//bool Node :: remove()
//{
//    if(m_pParent)
//    {
//        m_pParent->remove(this);
//        return true;
//    }
//    return false;
//}

glm::vec3 Node :: to_world(glm::vec3 point, Space s) const
{
    assert(s != Space::WORLD);
    std::queue<const Node*> ps;
    parents(ps, s == Space::LOCAL);
    while(!ps.empty())
    {
        point = Matrix::mult(*ps.front()->matrix_c(), point);
        ps.pop();
    }
    return point;
}

glm::vec3 Node :: from_world(glm::vec3 point, Space s) const
{
    assert(s != Space::WORLD);
    std::stack<const Node*> ps;
    parents(ps, s == Space::LOCAL);
    while(!ps.empty())
    {
        point = Matrix::mult(glm::inverse(*ps.top()->matrix_c()), point);
        ps.pop();
    }
    return point;
}

void Node :: cache_transform() const
{
    m_WorldTransform.ensure();
}

void Node :: each(std::function<void(Node*)> func)
{
    func(this);
    for(auto& c: m_Children)
        c->each(func);
}

void Node :: each(std::function<void(const Node*)> func) const
{
    func(this);
    for(const auto& c: m_Children)
        ((const Node*)c.get())->each(func);
}

const Box& Node :: world_box() const 
{
    return m_WorldBox();
}

Box Node :: calculate_world_box()
{
    if(m_Box.quick_full()) {
        return m_Box;
    } else if(m_Box.quick_zero()) {
        //assert(false);
        return m_Box;
    }
    
    Box world(Box::Zero());
    auto verts = m_Box.verts();
    
    for(auto& v: verts)
    {
        v = Matrix::mult(*matrix_c(Space::WORLD), v);
        world &= v;
    }
    return world;
}

