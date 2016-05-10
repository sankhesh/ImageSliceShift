#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkNew.h>
#include <vtkImplicitPlaneWidget2.h>
#include <vtkImplicitPlaneRepresentation.h>

int main(int, char *[])
{
  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.GetPointer());

  vtkNew<vtkImplicitPlaneRepresentation> rep;
  rep->SetPlaceFactor(1.25); // This must be set prior to placing the widget
  rep->PlaceWidget(actor->GetBounds());
  rep->SetNormal(plane->GetNormal());
  rep->SetOrigin(0,0,50); //this doesn't seem to work?

  vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetInteractor(renderWindowInteractor.GetPointer());
  planeWidget->SetRepresentation(rep.GetPointer());


  renderer->ResetCamera();
  renderWindowInteractor->Initialize();
  renderWindow->Render();
  planeWidget->On();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
