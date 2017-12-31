/*

  Written by Onur Paça, PhD student at Istanbul Technical University in 2017
  for the first homework of Advanced Mesh Processing Class instructued by 
  Asst. Prof. Dr. Erkan Günpınar

  paca[at]itu.edu.tr
  onurpaca[at]gmail.com
  opaca[at]rotorbit.com

*/

#include "SmootherRemesher.hh"

SmootherRemesherPlugin::SmootherRemesherPlugin() :
        iternoSpinbox_(0),
        targetEdgeLengthSpinbox_(0){
}

void SmootherRemesherPlugin::initializePlugin(){
  QWidget* toolBox = new QWidget();
  QPushButton* smoothButton = new QPushButton("&Smooth", toolBox);
  QPushButton* remeshButton = new QPushButton("&Remesh", toolBox);

  iternoSpinbox_ = new QSpinBox(toolBox);
  iternoSpinbox_->setMinimum(1);
  iternoSpinbox_->setMaximum(1000);
  iternoSpinbox_->setSingleStep(1);

  targetEdgeLengthSpinbox_ = new QDoubleSpinBox(toolBox);
  targetEdgeLengthSpinbox_->setMinimum(0.01);
  targetEdgeLengthSpinbox_->setMaximum(1000);
  targetEdgeLengthSpinbox_->setSingleStep(0.00001);

  QLabel* nIterLabel = new QLabel("Number of Iterations:");
  QLabel* targetEdgeLengthLabel = new QLabel("Target Edge Length:");

  QGridLayout* layout = new QGridLayout(toolBox);

  layout->addWidget(nIterLabel, 0, 0);
  layout->addWidget(iternoSpinbox_, 0, 1);
  layout->addWidget(smoothButton, 0, 2);

  layout->addWidget(targetEdgeLengthLabel, 1, 0);
  layout->addWidget(targetEdgeLengthSpinbox_, 1, 1);
  layout->addWidget(remeshButton, 1, 2);

  layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 0, 1, 2);

  connect(smoothButton, SIGNAL(clicked()), this, SLOT(smooth()));
  connect(remeshButton, SIGNAL(clicked()), this, SLOT(remesh()));
  emit addToolbox(tr("Smoother & Remesher"), toolBox);
}


void SmootherRemesherPlugin::smooth(){

  int nIter = iternoSpinbox_->value();  
  bool selectionExists = false;


  for (PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS); o_it != PluginFunctions::objectsEnd(); ++o_it) {

    if (o_it->dataType() == DATA_TRIANGLE_MESH){

      TriMesh* mesh = PluginFunctions::triMesh(*o_it);
      OpenMesh::VPropHandleT< TriMesh::Point > origPositions;
      mesh->add_property(origPositions, "SmootherPlugin_Original_Positions");

      for (int i=0 ; i<nIter; ++i) {

          TriMesh::VertexIter v_it, v_end=mesh->vertices_end();
          for (v_it=mesh->vertices_begin(); v_it!=v_end; ++v_it) {
            mesh->property( origPositions, *v_it ) = mesh->point(*v_it);
            selectionExists |= mesh->status(*v_it).selected();
          }

          for (v_it=mesh->vertices_begin(); v_it!=v_end; ++v_it) {
            if(selectionExists && mesh->status(*v_it).selected() == false) {
              continue;
            }

            TriMesh::Point point = TriMesh::Point(0.0,0.0,0.0);
            bool skip = false;
            TriMesh::VertexOHalfedgeIter voh_it(*mesh,*v_it);
            for ( ; voh_it.is_valid(); ++voh_it ) {
                point += mesh->property( origPositions, mesh->to_vertex_handle(*voh_it) );

                if ( mesh->is_boundary( *voh_it ) ) {
                  skip = true;
                  break;
                }

            }

            point /= mesh->valence( *v_it );

            if ( ! skip ) {
                mesh->point(*v_it) = point;
            }
          }
      }

      mesh->remove_property( origPositions );
      mesh->update_normals();
      emit updatedObject( o_it->id(), UPDATE_GEOMETRY );

    } 
 
    else if (o_it->dataType() == DATA_POLY_MESH){

      PolyMesh* mesh = PluginFunctions::polyMesh(*o_it);
      OpenMesh::VPropHandleT< TriMesh::Point > origPositions;
      mesh->add_property( origPositions, "SmootherPlugin_Original_Positions" );

      for (int i = 0 ; i<nIter; ++i) {

         PolyMesh::VertexIter v_it, v_end=mesh->vertices_end();
         for (v_it=mesh->vertices_begin(); v_it!=v_end; ++v_it) {
            mesh->property( origPositions, *v_it ) = mesh->point(*v_it);
            selectionExists |= mesh->status(*v_it).selected();
         }

         for (v_it=mesh->vertices_begin(); v_it!=v_end; ++v_it) {

            if(selectionExists && mesh->status(*v_it).selected() == false) {
              continue;
            }

            PolyMesh::Point point = PolyMesh::Point(0.0,0.0,0.0);

            bool skip = false;

            PolyMesh::VertexOHalfedgeIter voh_it(*mesh,*v_it);
            for ( ; voh_it.is_valid(); ++voh_it ) {
               point += mesh->property( origPositions, mesh->to_vertex_handle(*voh_it) );

               if ( mesh->is_boundary( *voh_it ) ) {
                  skip = true;
                  break;
               }

            }

            point /= mesh->valence( *v_it );

            if (!skip) {
               mesh->point(*v_it) = point;
            }
         }

      }


      mesh->remove_property( origPositions );
      mesh->update_normals();
      emit updatedObject( o_it->id(), UPDATE_GEOMETRY );

    }

    else {
      emit log(LOGERR, "DataType not supported.");
    }
    
    emit updatedObject(o_it->id(), UPDATE_ALL);

  } 

  emit updateView();
}



void SmootherRemesherPlugin::remesh(){

  double targetEdgeLength = targetEdgeLengthSpinbox_->value();  
  double low, high;

  std::cout << "Parsed Target Edge Length: " << targetEdgeLength << std::endl;

  for (PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS); o_it != PluginFunctions::objectsEnd(); ++o_it) {
    if (o_it->dataType() == DATA_TRIANGLE_MESH){

      TriMesh *mesh = PluginFunctions::triMesh(*o_it);
      OpenMesh::VPropHandleT< TriMesh::Point > origPositions;

      low = (4.0/5.0)*targetEdgeLength;
      high = (4.0/3.0)*targetEdgeLength;

      for (int i=0; i<10; i++){
        std::cout << std::endl;
        std::cout << "Iter No: " << i << std::endl;
        std::cout << "---------------------------------------------------" << std::endl;
        splitLongEdges(high, mesh);
        collapseShortEdges(high, low, mesh);
        equalizeValences(mesh);
        smooth(1, mesh);
      }

    }

    emit updatedObject(o_it->id(), UPDATE_ALL);

  }

  emit updateView();

}



void SmootherRemesherPlugin::splitLongEdges(double high, TriMesh* &mesh){

  std::cout << "Splitting Long Edges..." << std::endl;

  TriMesh::EdgeIter e_it, e_end = mesh->edges_end();
  double edgeLength;

  TriMesh::Point p1 = TriMesh::Point(0.0,0.0,0.0);
  TriMesh::Point p2 = TriMesh::Point(0.0,0.0,0.0);
  TriMesh::Point pc = TriMesh::Point(0.0,0.0,0.0);
  TriMesh::VertexHandle vh1, vh2, vhc;

  bool longEdgeExists = true;

  int iter = 0;
  while (longEdgeExists && iter<100){
    for (e_it = mesh->edges_begin(); e_it!=e_end; ++e_it) {
  
      vh1 = mesh->to_vertex_handle(mesh->halfedge_handle(*e_it, 0));
      vh2 = mesh->from_vertex_handle(mesh->halfedge_handle(*e_it, 0));
      p1 = mesh->point(vh1);
      p2 = mesh->point(vh2);
      pc = (p1 + p2)*0.5;
  
      edgeLength = mesh->calc_edge_length(e_it);
      if (edgeLength > high){
        vhc = mesh->split(*e_it, pc);
      }

    }

    longEdgeExists = checkLongEdges(high, mesh);
    iter++;
  }

}




void SmootherRemesherPlugin::collapseShortEdges(double high, double low, TriMesh* &mesh){

  std::cout << "Collapsing Short Edges..." << std::endl;
  
  TriMesh::HalfedgeIter he_it, he_end = mesh->halfedges_end();
  TriMesh::Point p1 = TriMesh::Point(0.0,0.0,0.0);
  TriMesh::Point p2 = TriMesh::Point(0.0,0.0,0.0);
  double edgeLength, distance;
  bool collapse_ok;
  TriMesh::VertexHandle vh1, vh2, vh;
  int iter = 0;
  int num_collapse = 0;
  bool shortEdgeExists = true; 

  //while (shortEdgeExists && iter<100){
  //std::cout << "Collapse Short Edge Iter: " << iter << std::endl;

  for (he_it = mesh->halfedges_begin(); he_it!=he_end; ++he_it) {

    collapse_ok = false;  

    edgeLength = mesh->calc_edge_length(he_it);
    if (edgeLength < low){
      vh1 = mesh->from_vertex_handle(*he_it);
      vh2 = mesh->to_vertex_handle(*he_it);
      p1 = mesh->point(vh2); 

      TriMesh::VertexVertexIter vv_it = mesh->vv_iter(vh1);
      collapse_ok = true;

      for (; vv_it; ++vv_it){
        vh = mesh->vertex_handle(vv_it);
        p2 = mesh->point(vh);
        distance = sqrt(pow((p1[0]-p2[0]),2) + pow((p1[1]-p2[1]),2) + pow((p1[2]-p2[2]),2));
        if (distance > high){
          collapse_ok = false;
        }
      }

    }

    if (collapse_ok && !mesh->is_boundary(he_it)){
      mesh->collapse(*he_it);
      num_collapse++;
    }
  }

  //shortEdgeExists = checkShortEdges(low, mesh);
  //iter++;
  //}

}





void SmootherRemesherPlugin::equalizeValences(TriMesh* &mesh){
  
  std::cout << "Equalizing Valences..." << std::endl;

  TriMesh::HalfedgeIter he_it, he_end = mesh->halfedges_end();
  TriMesh::FaceHandle fh1, fh2;
  TriMesh::HalfedgeHandle he1, he2;
  TriMesh::FaceVertexIter fv_it1, fv_it2;
  TriMesh::VertexHandle vh1, vh2;
  TriMesh::EdgeHandle eh;

  double deviation_pre = 0.0;
  double deviation_post = 0.0;

    for (he_it = mesh->halfedges_begin(); he_it!=he_end; ++he_it) {
      fh1 = mesh->face_handle(he_it);
      he2 = mesh->opposite_halfedge_handle(he_it);
      fh2 = mesh->face_handle(he2);

      fv_it1 = mesh->fv_iter(fh1); 
      fv_it2 = mesh->fv_iter(fh2); 

      for (; fv_it1; ++fv_it1){
        deviation_pre += abs(mesh->valence(fv_it1) - 6.0);
      }

      for (; fv_it2; ++fv_it2){
        deviation_pre += abs(mesh->valence(fv_it2) - 6.0);
      }

      vh1 = mesh->from_vertex_handle(*he_it);
      vh2 = mesh->from_vertex_handle(*he_it);

      deviation_pre -= abs(mesh->valence(vh1) - 6.0);
      deviation_pre -= abs(mesh->valence(vh2) - 6.0);
      eh = mesh->edge_handle(he_it);
      
      if (mesh->is_boundary(eh))
        continue;

      mesh->flip(eh);

      for (; fv_it1; ++fv_it1){
        deviation_post += abs(mesh->valence(fv_it1) - 6.0);
      }

      for (; fv_it2; ++fv_it2){
        deviation_post += abs(mesh->valence(fv_it2) - 6.0);
      }

      deviation_post -= abs(mesh->valence(vh1) - 6.0);
      deviation_post -= abs(mesh->valence(vh2) - 6.0).;

      if (deviation_pre <= deviation_post)
        mesh->flip(eh);

    }

}





bool SmootherRemesherPlugin::checkLongEdges(double high, TriMesh* &mesh){

  TriMesh::EdgeIter e_it, e_end = mesh->edges_end();
  double edgeLength;
  bool longEdgeExists = false;

  for (e_it = mesh->edges_begin(); e_it!=e_end; ++e_it) {
    edgeLength = mesh->calc_edge_length(e_it);
    if (edgeLength > high){
      longEdgeExists = true;
    }
  }
  return longEdgeExists;
}




bool SmootherRemesherPlugin::checkShortEdges(double low, TriMesh* &mesh){

  TriMesh::EdgeIter e_it, e_end = mesh->edges_end();
  double edgeLength;
  bool shortEdgeExists = false;

  for (e_it = mesh->edges_begin(); e_it!=e_end; ++e_it) {
    edgeLength = mesh->calc_edge_length(e_it);
    if (edgeLength < low){
      shortEdgeExists = true;
    }
  }
  return shortEdgeExists;
}



void SmootherRemesherPlugin::smooth(int nIter, TriMesh* &mesh){
  
  std::cout << "Smoothing Mesh..." << std::endl;

  bool selectionExists = false;


  for (PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS); o_it != PluginFunctions::objectsEnd(); ++o_it) {


    OpenMesh::VPropHandleT< TriMesh::Point > origPositions;
    mesh->add_property(origPositions, "SmootherPlugin_Original_Positions");

    for (int i=0 ; i<nIter; ++i) {

        TriMesh::VertexIter v_it, v_end=mesh->vertices_end();
        for (v_it=mesh->vertices_begin(); v_it!=v_end; ++v_it) {
          mesh->property( origPositions, *v_it ) = mesh->point(*v_it);
          selectionExists |= mesh->status(*v_it).selected();
        }

        for (v_it=mesh->vertices_begin(); v_it!=v_end; ++v_it) {
          if(selectionExists && mesh->status(*v_it).selected() == false) {
            continue;
          }

          TriMesh::Point point = TriMesh::Point(0.0,0.0,0.0);
          bool skip = false;
          TriMesh::VertexOHalfedgeIter voh_it(*mesh,*v_it);
          for ( ; voh_it.is_valid(); ++voh_it ) {
              point += mesh->property( origPositions, mesh->to_vertex_handle(*voh_it) );

              if ( mesh->is_boundary( *voh_it ) ) {
                skip = true;
                break;
              }

          }

          point /= mesh->valence( *v_it );

          if ( ! skip ) {
              mesh->point(*v_it) = point;
          }
        }
    }

    mesh->remove_property( origPositions );
    mesh->update_normals();
    emit updatedObject( o_it->id(), UPDATE_GEOMETRY );

 
    
    emit updatedObject(o_it->id(), UPDATE_ALL);

  } 

  emit updateView();
}





Q_EXPORT_PLUGIN2(smootherremeshplugin, SmootherRemesherPlugin);
