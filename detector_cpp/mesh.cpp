#include "mesh.h"

Point_3D &Point_3D::operator=(Point_3D &_other)
{
    x = _other.x;
    y = _other.y;
    z = _other.z;
    return *this;
}

ostream &operator<<(ostream &os, Point_3D &_other)
{
    os << "Point at (" << _other.x << ", " << _other.y << ", " << _other.z << ")";
    return os;
}

Vertex::Vertex(Point_3D _pos, indx _index, bool _isInterestPoint)
{
    m_pos = _pos;
    index = _index;
    depth = 0;
    isInterestPoint = _isInterestPoint;
    response = 0.0;
}

// Vertex& Vertex::operator=(Vertex& _vertex)
// {
//     m_pos = _vertex.m_pos;
//     index = _vertex.index;
//     depth = _vertex.depth;
//     response = _vertex.response;
//     isMarked = _vertex.isMarked;
//     isInterestPoint = _vertex.isInterestPoint;
//     m_faces = _vertex.m_faces;
//     m_adjacentVertices = _vertex.m_adjacentVertices;
//     return *this;
// }

void Vertex::ring(int k, vector<Vertex *> &_neighborhood, Vertex *vertices)
{
    int _depth = depth = 0;

    queue<Vertex*> _queue;
    vector<Vertex*> _marked;
    _queue.push(this);      //Put the vertex in the queue
    _marked.push_back(this);
    isMarked = true;        //Mark vertex as navigated

    Vertex* _qv0;
    while(!_queue.empty()) {
        _qv0 = _queue.front();    _queue.pop();
        _neighborhood.push_back(new Vertex(Point_3D(_qv0->m_pos)));

        _depth = _qv0->depth;
        if(_depth <= k) {
            ///****Esto puede ser mejorado con una diferencia de sets****///
//            set<indx> _neighbours = _qv0->m_adjacentVertices;
//            set<indx>::iterator it0 = _neighbours.begin(),
//                                itf = _neighbours.end();
            set<indx>::iterator it0 = _qv0->m_adjacentVertices.begin(),
                                itf = _qv0->m_adjacentVertices.end();
            Vertex* _vert;
            for(; it0 != itf; it0++) {
                _vert = &vertices[*it0];
                if(!_vert->isMarked) {
                    _queue.push(_vert);
                    _marked.push_back(_vert);
                    _vert->isMarked = true;
                    _vert->depth = _depth + 1;
                }
            }
        }
    }

    for(vector<Vertex*>::iterator it = _marked.begin();
        it < _marked.end(); it++) {
        (*it)->isMarked = false;
        (*it)->depth = 0;
    }
}

void Vertex::defineInterest(Vertex *_vertices)
{
    set<indx>::iterator it = m_adjacentVertices.begin(),
                        it2 = m_adjacentVertices.end();
    Vertex* _tmp;
    for(; it != it2; it++) {
        _tmp = &_vertices[*it];
        if(_tmp != this && response < _tmp->response)
            return;
    }
    isInterestPoint = true;
}

ostream &operator<<(ostream &os, Vertex &_vertex)
{
    os << "Vertex #" << _vertex.index << " @ " << _vertex.m_pos << "\tIs Interest?: " << _vertex.isInterestPoint;
    return os;
}

void Face::add(Vertex::indx _index)
{
    m_vertices.push_back(_index);
}

vector<Vertex::indx> &Face::getVertices()
{
    return m_vertices;
}

Mesh::Mesh() :
    m_vertices(NULL),
    m_faces(NULL)
{}

void Mesh::reset()
{
    if(m_faces) {
        delete[] m_faces;
        m_faces = NULL;
    }

    if(m_vertices) {
        delete[] m_vertices;
        m_vertices = NULL;
    }

    m_nVertices = m_nFaces = 0;
}

void Mesh::loadOffFile(string filename)
{
    cout << "\nWill load " << filename << " now!" << endl;

    ifstream _file;
    _file.open(filename.c_str()/*, ios::in*/);

    string _format;

    int register i, _nVertices;
    _file >> _format >> m_nVertices >> m_nFaces >> i;

    if(_format.compare("OFF") != 0) {
        cout << "Not an OFF file" << endl;
        return;
    }

    m_vertices = new Vertex[m_nVertices];
    m_faces = new Face[m_nFaces];

    // Fill the cloud with the model's points
    double _x, _y, _z;
    for(i = 0; i < m_nVertices; i++) {
        _file   >> _x
                >> _y
                >> _z;

        m_vertices[i].m_pos.x = _x;
        m_vertices[i].m_pos.y = _y;
        m_vertices[i].m_pos.z = _z;

        m_vertices[i].index = i;
    }

    size_t _indx1, _indx2, _indx3;

    for(i = 0; i < m_nFaces; i++) {
        _file >> _nVertices     //Grabs the 'n' number of nodes in a face
                >> _indx1
                >> _indx2
                >> _indx3;

        if(_nVertices != 3) {
            cout << "Not triangles\n";
            break;
        }

        //Set indexes of the face
        m_faces[i].add(_indx1);
        m_faces[i].add(_indx2);
        m_faces[i].add(_indx3);

        //Add faces to vertices
        m_vertices[_indx1].addFace(i);
        m_vertices[_indx2].addFace(i);
        m_vertices[_indx3].addFace(i);

        //Add adjacent vertices to vertex
        m_vertices[_indx1].addVertex(_indx2);
        m_vertices[_indx1].addVertex(_indx3);

        m_vertices[_indx2].addVertex(_indx1);
        m_vertices[_indx2].addVertex(_indx3);

        m_vertices[_indx3].addVertex(_indx1);
        m_vertices[_indx3].addVertex(_indx2);
    }

    cout << filename << " was loaded succesfully..." << endl;
}

void Mesh::loadMatFile(string filename)
{
    mat_t *mat = Mat_Open(filename.c_str(), MAT_ACC_RDONLY);
    matvar_t *matvar;
    
    if(!mat) {
        cout << "Couldn't load .mat file'" ;
        return;
    }

    // while ( (matvar = Mat_VarReadNextInfo(mat)) != NULL ) {
    //     printf("%s\n",matvar->name);

    //     Mat_VarFree(matvar);
    //     matvar = NULL;
    // }
    
    //////////////VERTICES////////////////////
    matvar = Mat_VarRead(mat, (char*)"V");

    if(!matvar) {
        cout << "Couldn't load vertices, check your file\n";
        return;
    }

    m_nVertices = matvar->dims[0];
    
    if(matvar->dims[1] != 3) {
        cout << "Not triangles\n";
        return;
    }

    m_vertices = new Vertex[m_nVertices];


    for(int i=0; i<matvar->rank; ++i) {
        cout<<"\tdim["<<i<<"] == "<<matvar->dims[i]<<"\n";
    }

    char A;
    cin >> A;

    const double *VData = static_cast<const double*>(matvar->data) ;
    for(register int i = 0, j; i < m_nVertices; ++i) {
        j = i * 3;
        m_vertices[i].m_pos.x = VData[  j];
        m_vertices[i].m_pos.y = VData[j+1];
        m_vertices[i].m_pos.z = VData[j+2];
        m_vertices[i].index   = i;
    }

    Mat_VarFree(matvar);
    matvar = NULL;

    ///////////////FACES/////////////////////
    matvar = Mat_VarRead(mat, (char*)"F");

    if(!matvar) {
        cout << "Couldn't load faces, check your file\n";
        return;
    }

    m_nFaces = matvar->dims[0];
    m_faces = new Face[m_nFaces];

    size_t _indx1, _indx2, _indx3;
    const double *FData = static_cast<const double*>(matvar->data);
    for(register int i = 0, j; i < m_nFaces; ++i) {
//    for(register int i = 0, j; i < 30; ++i) {
        j = i * 3;
        _indx1 = FData[j] -1;  _indx2 = FData[j+1] -1;    _indx3 = FData[j+2] -1;
//        cout<<"\tF["<< i << ", 1] = "<<FData[j]<<"\n";
//        cout<<"\tF["<< i << ", 2] = "<<FData[j+1]<<"\n";
//        cout<<"\tF["<< i << ", 3] = "<<FData[j+2]<<"\n";

//        cout<<"F["<< i << ", 1] = "<<FData[i]<<"\t";
//        cout<<"F["<< i << ", 2] = "<<FData[i*2]<<"\t";
//        cout<<"F["<< i << ", 3] = "<<FData[i*3]<<"\n";

        //Set indexes of the face
        m_faces[i].add(_indx1);
        m_faces[i].add(_indx2);
        m_faces[i].add(_indx3);

        //Add faces to vertices
        m_vertices[_indx1].addFace(i);
        m_vertices[_indx2].addFace(i);
        m_vertices[_indx3].addFace(i);

        cout << "I1: " << _indx1 << "\tI2: " << _indx2 << "\tI3: " << _indx3 << endl;

        //Add adjacent vertices to vertex
            m_vertices[_indx1].addVertex(_indx2);
            m_vertices[_indx1].addVertex(_indx3);

            m_vertices[_indx2].addVertex(_indx1);
            m_vertices[_indx2].addVertex(_indx3);

            m_vertices[_indx3].addVertex(_indx1);
            m_vertices[_indx3].addVertex(_indx2);
    }

    Mat_VarFree(matvar);
    matvar = NULL;

    //////////////CERRAR EL ARCHIVO/////////////////


    // std::cout<<"y:\n" ;
    // matVar = Mat_VarRead(mat, (char*)"y") ;
    // unsigned ySize = matVar->nbytes/matVar->data_size ;
    // const double *yData = static_cast<const double*>(matVar->data) ;
    // for(int i=0; i<ySize; ++i)
    // {
    //     double d = yData[i] ;
    //     std::cout<<"\ty["<<i<<"] = "<<d<<"\n" ;
    // }
    // std::cout<<"\n" ;
    // for(int i=0; i<matVar->rank; ++i)
    // {
    //     std::cout<<"\tdim["<<i<<"] == "<<matVar->dims[i]<<"\n" ;
    // }
    // std::cout<<"minX, step:\n";
    // matVar = Mat_VarRead(mat, (char*)"minX") ;
    // const double *minXData = static_cast<const double*>(matVar->data) ;
    // std::cout<<"\tminX = "<<minXData[0]<<"\n" ;
    // std::cout<<"\n" ;
    // for(int i=0; i<matVar->rank; ++i)
    // {
    //     std::cout<<"\tdim["<<i<<"] == "<<matVar->dims[i]<<"\n" ;
    // }
    // matVar = Mat_VarRead(mat, (char*)"step") ;
    // const double *stepData = static_cast<const double*>(matVar->data) ;
    // std::cout<<"\tstep = "<<stepData[0]<<"\n" ;
    // std::cout<<"\n" ;
    // for(int i=0; i<matVar->rank; ++i)
    // {
    //     std::cout<<"\tdim["<<i<<"] == "<<matVar->dims[i]<<"\n" ;
    // }

    Mat_Close(mat);
}
