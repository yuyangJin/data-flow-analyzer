#ifndef GRAPH_H_
#define GRAPH_H_

#include <igraph.h>
//#include <fstream>
#include <map>
#include <memory>
//#include "nlohmann/json.hpp"
//#include <stack>
#include <string>
//#include <tuple>
//#include <unordered_set>
#include <vector>
//#include "common/common.h"

#define TRUNK_SIZE 1000

namespace depdetector::type {
struct graph_t {                        /**<igragh-graph wrapper */
  igraph_t graph;
};

struct vertex_set_t {                    /**<igraph_vs_t wrapper */
  igraph_vs_t vertices;
};
typedef int vertex_t;                  /**<vertex id type (int)*/
typedef int edge_t;                    /**<edge id type (int)*/
typedef unsigned long long int addr_t; /**<address type (unsigned long long int)*/
typedef double num_t;
}

namespace depdetector {

/** @brief Wrapper class of igraph. Provide basic graph operations.
    @author Yuyang Jin, PACMAN, Tsinghua University
    @date March 2021
    */
class Graph {
 protected:
  std::unique_ptr<type::graph_t> ipag_; /**<igraph_t wrapper struct */
  int cur_vertex_num;                   /**<initial the number of vertices in this graph */
  //GraphPerfData* graph_perf_data;       /**<performance data in a graph*/

 public:
  /** Constructor. Create an graph and enable graph attributes.
   */
  Graph();

  /** Destructor. Destroy graph.
   */
  ~Graph();

  /** Initialize the graph. Build a directed graph with zero vertices and zero edges. Set name of the graph with the
   * input parameter.
   * @param graph_name - name of the graph
   */
  void GraphInit(const char* graph_name);

  /** Create a vertex in the graph.
   * @return id of the new vertex
   */
  type::vertex_t AddVertex();

  /** Create an edge in the graph.
   * @param src_vertex_id - id of the source vertex of the edge
   * @param dest_vertex_id - id of the destination vertex of the edge
   * @return id of the new edge
   */
  type::edge_t AddEdge(const type::vertex_t src_vertex_id, const type::vertex_t dest_vertex_id);

  /** Append a graph to the graph. Copy all the vertices and edges (and all their attributes) of a graph to this graph.
   * @param g - the graph to be appended
   * @return id of entry vertex of the appended new graph (not id in old graph)
   */
  type::vertex_t AddGraph(Graph* g);

  /** Delete a vertex.
   * @param vertex_id - id of vertex to be removed
   */
  void DeleteVertex(type::vertex_t vertex_id);

  /** Delete a set of vertex.
   * @param vs - set of vertices to be deleted
   */
  void DeleteVertices(depdetector::type::vertex_set_t* vs);

  /** Delete extra vertices at the end of vertices. (No need to expose to developers)
   */
  void DeleteExtraTailVertices();

  /** Delete an edge.
   * @param src_vertex_id - id of the source vertex of the edge to be removed
   * @param dest_vertex_id - id of the destination vertex of the edge to be removed
   */
  void DeleteEdge(type::vertex_t src_vertex_id, type::vertex_t dest_vertex_id);

  /** Swap two vertices. Swap all attributes except for "id"
   * @param vertex_id_1 - id of the first vertex
   * @param vertex_id_2 - id of the second vertex
   */
  void SwapVertex(type::vertex_t vertex_id_1, type::vertex_t vertex_id_2);

  /** Query a vertex. (Not implement yet.)
   */
  void QueryVertex();

  /** Query an edge with source and destination vertex ids.
   * @param src_vertex_id - id of the source vertex of the edge
   * @param dest_vertex_id - id of the destination vertex of the edge
   * @return id of the queried edge
   */
  type::edge_t QueryEdge(type::vertex_t src_vertex_id, type::vertex_t dest_vertex_id);

  /** Get the source vertex of the input edge.
   * @param edge_id - input edge id
   * @return id of the source vertex
   */
  type::vertex_t GetEdgeSrc(type::edge_t edge_id);

  /** Get the destination vertex of the input edge.
   * @param edge_id - input edge id
   * @return id of the destination vertex
   */
  type::vertex_t GetEdgeDest(type::edge_t edge_id);

  /** Get the other side vertex of the input edge. (Not implement yet.)
   * @param edge_id - input edge id
   * @param vertex_id - input vertex id
   * @return id of the vertex in the other side of the input edge
   */
  void GetEdgeOtherSide();

  /** Checks whether a graph attribute exists.
   *  Time complexity: O(A), the number of graph attributes, assuming attribute names are not too long.
   * @param attr_name - name of the graph attribute
   * @return Logical value, TRUE if the attribute exists, FALSE otherwise.
  */
  bool HasGraphAttribute(const char* attr_name);

  /** Checks whether a vertex attribute exists.
   *  Time complexity: O(A), the number of vertex attributes, assuming attribute names are not too long.
   * @param attr_name - name of the graph attribute
   * @return Logical value, TRUE if the attribute exists, FALSE otherwise.
  */
  bool HasVertexAttribute(const char* attr_name);

  /** Checks whether a edge attribute exists.
   *  Time complexity: O(A), the number of edge attributes, assuming attribute names are not too long.
   * @param attr_name - name of the graph attribute
   * @return Logical value, TRUE if the attribute exists, FALSE otherwise.
  */
  bool HasEdgeAttribute(const char* attr_name);

  /** Set a string graph attribute
   * @param attr_name - name of the graph attribute
   * @param value - the (new) value of the graph attribute
   */
  void SetGraphAttributeString(const char* attr_name, const char* value);

  /** Set a numeric graph attribute
   * @param attr_name - name of the graph attribute
   * @param value - the (new) value of the graph attribute
   */
  void SetGraphAttributeNum(const char* attr_name, const type::num_t value);

  /** Set a boolean graph attribute as flag
   * @param attr_name - name of the graph attribute
   * @param value - the (new) value of the graph attribute
   */
  void SetGraphAttributeFlag(const char* attr_name, const bool value);

  /** Set a string vertex attribute
   * @param attr_name - name of the vertex attribute
   * @param vertex_id - the vertex id
   * @param value - the (new) value of the vertex attribute
   */
  void SetVertexAttributeString(const char* attr_name, type::vertex_t vertex_id, const char* value);

  /** Set a numeric vertex attribute
   * @param attr_name - name of the vertex attribute
   * @param vertex_id - the vertex id
   * @param value - the (new) value of the vertex attribute
   */
  void SetVertexAttributeNum(const char* attr_name, type::vertex_t vertex_id, const type::num_t value);

  /** Set a boolean vertex attribute as flag
   * @param attr_name - name of the vertex attribute
   * @param vertex_id - the vertex id
   * @param value - the (new) value of the vertex attribute
   */
  void SetVertexAttributeFlag(const char* attr_name, type::vertex_t vertex_id, const bool value);

  /** Set a string edge attribute
   * @param attr_name - name of the edge attribute
   * @param type::edge_t - the edge id
   * @param value - the (new) value of the edge attribute
   */
  void SetEdgeAttributeString(const char* attr_name, type::edge_t edge_id, const char* value);

  /** Set a numeric edge attribute
   * @param attr_name - name of the edge attribute
   * @param type::edge_t - the edge id
   * @param value - the (new) value of the edge attribute
   */
  void SetEdgeAttributeNum(const char* attr_name, type::edge_t edge_id, const type::num_t value);

  /** Set a boolean edge attribute as flag
   * @param attr_name - name of the edge attribute
   * @param type::edge_t - the edge id
   * @param value - the (new) value of the edge attribute
   */
  void SetEdgeAttributeFlag(const char* attr_name, type::edge_t edge_id, const bool value);

  /** Get a string graph attribute
   * @param attr_name - name of the graph attribute
   * @return the (new) value of the graph attribute
   */
  const char* GetGraphAttributeString(const char* attr_name);

  /** Get a numeric graph attribute
   * @param attr_name - name of the graph attribute
   * @return the (new) value of the graph attribute
   */
  const type::num_t GetGraphAttributeNum(const char* attr_name);

  /** Get a flag graph attribute
   * @param attr_name - name of the graph attribute
   * @return the (new) value of the graph attribute
   */
  const bool GetGraphAttributeFlag(const char* attr_name);

  /** Get a string vertex attribute
   * @param attr_name - name of the vertex attribute
   * @param vertex_id - the vertex id
   * @return value - the (new) value of the vertex attribute
   */
  const char* GetVertexAttributeString(const char* attr_name, type::vertex_t vertex_id);

  /** Get a numeric vertex attribute
   * @param attr_name - name of the vertex attribute
   * @param vertex_id - the vertex id
   * @return value - the (new) value of the vertex attribute
   */
  const type::num_t GetVertexAttributeNum(const char* attr_name, type::vertex_t vertex_id);

  /** Get a flag vertex attribute
   * @param attr_name - name of the vertex attribute
   * @param vertex_id - the vertex id
   * @return value - the (new) value of the vertex attribute
   */
  const bool GetVertexAttributeFlag(const char* attr_name, type::vertex_t vertex_id);

  /** Get a string edge attribute
   * @param attr_name - name of the edge attribute
   * @param type::edge_t - the edge id
   * @return value - the (new) value of the edge attribute
   */
  const char* GetEdgeAttributeString(const char* attr_name, type::edge_t edge_id);

  /** Get a numeric edge attribute
   * @param attr_name - name of the edge attribute
   * @param type::edge_t - the edge id
   * @return value - the (new) value of the edge attribute
   */
  const type::num_t GetEdgeAttributeNum(const char* attr_name, type::edge_t edge_id);

  /** Get a flag edge attribute
   * @param attr_name - name of the edge attribute
   * @param type::edge_t - the edge id
   * @return value - the (new) value of the edge attribute
   */
  const bool GetEdgeAttributeFlag(const char* attr_name, type::edge_t edge_id);

  /** Remove a graph attribute
   * @param attr_name - name of the graph attribute
   */
  void RemoveGraphAttribute(const char* attr_name);

  /** Remove a vertex attribute
   * @param attr_name - name of the vertex attribute
   */
  void RemoveVertexAttribute(const char* attr_name);

  /** Remove an edge attribute
   * @param attr_name - name of the edge attribute
   */
  void RemoveEdgeAttribute(const char* attr_name);

  void MergeVertices();
  void SplitVertex();

  /** Copy a vertex to the designated vertex. All attributes (include "id") are copied.
   * @param new_vertex_id - id of the designated vertex
   * @param g - graph that contains the vertex to be copied
   * @param vertex_id - id of the vertex to be copied
   */
  void DeepCopyVertex(type::vertex_t new_vertex_id, Graph* g, type::vertex_t vertex_id);

  /** Copy a vertex to the designated vertex. All attributes, except "id", are copied.
   * @param new_vertex_id - id of the designated vertex
   * @param g - graph that contains the vertex to be copied
   * @param vertex_id - id of the vertex to be copied
   */
  void CopyVertex(type::vertex_t new_vertex_id, Graph* g, type::vertex_t vertex_id);
  // TODO: do not expose inner igraph

  /** Depth-First Search, not implement yet.
   */
  void Dfs();

  /** Read a graph from a GML format file.
   * @param file_name - name of input file
   */
  void ReadGraphGML(const char* file_name);

  /** Dump the graph as a GML format file.
   * @param file_name - name of output file
   */
  void DumpGraphGML(const char* file_name);

  /** Dump the graph as a dot format file.
   * @param file_name - name of output file
   */
  void DumpGraphDot(const char* file_name);

  /** Get the number of vertices.
   * @return the number of vertices
   */
  int GetCurVertexNum();

  /** Get a set of child vertices.
   * @param vertex_id - id of a vertex
   * @param child_vec - a vector that stores the id of child vertices
   */
  void GetChildVertexSet(type::vertex_t vertex_id, std::vector<type::vertex_t>& child_vec);

  /** Get parent vertex.
   * @param vertex_id - id of a vertex
   * @return parent vertex of input vertex
   *
  */
  type::vertex_t GetParentVertex(type::vertex_t vertex_id);

  /** [Graph Algorithm] Traverse all vertices and execute CALL_BACK_FUNC when accessing each vertex.
   * @param CALL_BACK_FUNC - callback function when a vertex is accessed. The input parameters of this function contain
   * a pointer to the graph being traversed, id of the accessed vertex, and an extra pointer for developers to pass more
   * parameters.
   * @param extra - a pointer for developers to pass more parameters as the last parameter of CALL_BACK_FUNC
   */
  void VertexTraversal(void (*CALL_BACK_FUNC)(Graph*, type::vertex_t, void*), void* extra);

  /** [Graph Algorithm] Perform Pre-order traversal on the graph.
   * @param root_vertex_id - id of the starting vertex
   * @param pre_order_vertex_vec - a vector that stores the accessing sequence (id) of vertex
   */
  void PreOrderTraversal(type::vertex_t root_vertex_id, std::vector<type::vertex_t>& pre_order_vertex_vec);

  /** [Graph Algorithm] Perform Depth-First Search on the graph.
   * @param root - The id of the root vertex.
   * @param IN_CALL_BACK_FUNC - callback function when a new vertex is discovered / accessed. The input parameters of
   * this function contain
   * a pointer to the graph being traversed, id of the accessed vertex, and an extra pointer for developers to pass more
   * parameters.
   * @param OUT_CALL_BACK_FUNC - callback function when the subtree of a vertex is completed. The input parameters of
   * this function contain
   * a pointer to the graph being traversed, id of the accessed vertex, and an extra pointer for developers to pass more
   * parameters.
   * @param extra - a pointer for developers to pass more parameters as the last parameter of IN_CALL_BACK_FUNC and
   * OUT_CALL_BACK_FUNC
  */
  void DFS(type::vertex_t root, void (*IN_CALL_BACK_FUNC)(Graph*, int, void*),
           void (*OUT_CALL_BACK_FUNC)(Graph*, int, void*), void* extra);

  /** [Graph Algorithm] Perform Breadth-First Search on the graph.
   * @param root - The id of the root vertex.
   * @param CALL_BACK_FUNC - callback function when a new vertex is discovered / accessed. The input parameters of
   * this function contain a pointer to the graph being traversed, id of the accessed vertex, and an extra pointer for
   * developers to pass more
   * parameters.
   * @param extra - a pointer for developers to pass more parameters as the last parameter of CALL_BACK_FUNC
  */
  void BFS(type::vertex_t root, void (*CALL_BACK_FUNC)(Graph*, int, void*), void* extra);

  //GraphPerfData* GetGraphPerfData();

  /** Sort child vertices from a starting vertex recursively
   * @param starting_vertex - a starting vertex.
   * @param attr_name - a key attribute to sort by.
  */
  void SortBy(type::vertex_t starting_vertex, const char* attr_name);
};

}  // namespace depdetector

#endif  // GRAPH_H_