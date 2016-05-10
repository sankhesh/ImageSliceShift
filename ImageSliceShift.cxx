#include <vtkColorTransferFunction.h>
#include <vtkDICOMImageReader.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkImplicitPlaneWidget2.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

int main(int argc, char *argv[])
{
  vtkNew<vtkDICOMImageReader> reader;
  reader->SetDirectoryName(argv[1]);
  reader->Update();

  vtkImageData* im = reader->GetOutput();
  double range[2];
  im->GetScalarRange(range);
  int dims[3], extent[6];
  double spacing[3], origin[3], bounds[6];
  im->GetDimensions(dims);
  im->GetExtent(extent);
  im->GetSpacing(spacing);
  im->GetOrigin(origin);
  im->GetBounds(bounds);

  std::cout << "Dimensions: " << dims[0] << " " <<
    dims[1] << " " << dims[2] << std::endl;
  std::cout << "Origin: " << origin[0] << " " <<
    origin[1] << " " << origin[2] << std::endl;
  std::cout << "Spacing: " << spacing[0] << " " <<
    spacing[1] << " " << spacing[2] << std::endl;
  std::cout << "Bounds: " <<
    bounds[0] << " " << bounds[1] <<  " " <<
    bounds[2] << " " << bounds[3] <<  " " <<
    bounds[4] << " " << bounds[5] << std::endl;


  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(324.13, 1.0, 0.2, 0.2);
  ctf->AddRGBPoint(510.14, 1.0, 0.2, 0.2);
  ctf->AddRGBPoint(696.15, 0.9, 0.9, 0.9);
  ctf->AddRGBPoint(714.75, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(785.43, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(range[1], 1.0, 1.0, 1.0);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(range[0], 0.0);
  pf->AddPoint(324.13, 0.0);
  pf->AddPoint(510.14, 0.5);
  pf->AddPoint(696.15, 0.5);
  pf->AddPoint(714.75, 0.7);
  pf->AddPoint(785.43, 1.0);
  pf->AddPoint(range[1], 1.0);

  vtkNew<vtkVolumeProperty> property;
  property->ShadeOn();
  property->SetColor(ctf.GetPointer());
  property->SetScalarOpacity(pf.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(property.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->SetViewport(0, 0, 0.5, 1);
  renderer->AddVolume(volume.GetPointer());

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 400);
  renderWindow->AddRenderer(renderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.GetPointer());

  vtkNew<vtkPlane> plane;

  vtkNew<vtkImplicitPlaneRepresentation> rep;
  rep->SetPlaceFactor(1.25); // This must be set prior to placing the widget
  rep->PlaceWidget(volume->GetBounds());
  rep->SetNormal(plane->GetNormal());
  rep->SetOrigin(0,0,50); //this doesn't seem to work?

  vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetInteractor(renderWindowInteractor.GetPointer());
  planeWidget->SetRepresentation(rep.GetPointer());

  renderer->ResetCamera();
  renderWindowInteractor->Initialize();
  planeWidget->On();
  renderWindow->Render();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
