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
#include <vtkImageActor.h>
#include <vtkImageReslice.h>
#include <vtkImageMapper3D.h>
#include <vtkPlaneSource.h>
#include <vtkMatrix4x4.h>

class vtkISSCallback : public vtkCommand
{
public:
  static vtkISSCallback * New()
    {
    return new vtkISSCallback;
    }

  virtual void Execute(vtkObject* caller, unsigned long, void*)
    {
    vtkImplicitPlaneWidget2 * planeWidget =
      reinterpret_cast<vtkImplicitPlaneWidget2*> (caller);
    vtkImplicitPlaneRepresentation * rep =
      reinterpret_cast<vtkImplicitPlaneRepresentation*>
      (planeWidget->GetRepresentation());
    rep->GetPlane(this->Plane);

    double * bounds = rep->GetBounds();

    // Slice
    double origin[3], normal[3];
    this->Plane->GetOrigin(origin);
    this->Plane->GetNormal(normal);
    vtkNew<vtkPlaneSource> s;
    s->SetNormal(normal);
    s->SetOrigin(origin);
    s->Update();

    double x[3] = {1, 0, 0};
    double y[3] = {0, 1, 0};
    double xproj[3], yproj[3];
    this->Plane->ProjectPoint(x, xproj);
    this->Plane->ProjectPoint(y, yproj);
    std::cout << "normal: " << normal[0] << " " << normal[1] << " " << normal[2] << std::endl;
    std::cout << "origin: " << origin[0] << " " << origin[1] << " " << origin[2] << std::endl;
    std::cout << "xproj: " << xproj[0] << " " << xproj[1] << " " << xproj[2] << std::endl;
    std::cout << "yproj: " << yproj[0] << " " << yproj[1] << " " << yproj[2] << std::endl;

    double v1[3], v2[3];
    vtkMath::Subtract(xproj, origin, v1);
    vtkMath::Subtract(yproj, origin, v2);

    vtkNew<vtkMatrix4x4> resliceAxes;
    resliceAxes->Identity();

    for ( int i = 0; i < 3; i++ )
      {
      resliceAxes->SetElement(0,i,v1[i]);
      resliceAxes->SetElement(1,i,v2[i]);
      resliceAxes->SetElement(2,i,normal[i]);
      }

    double planeOrigin[4];
    this->Plane->GetOrigin(planeOrigin);
    planeOrigin[3] = 1.0;
    double originXYZW[4];
    double neworiginXYZW[4];

    resliceAxes->MultiplyPoint(planeOrigin, originXYZW);
    resliceAxes->Transpose();
    resliceAxes->MultiplyPoint(originXYZW, neworiginXYZW);

    resliceAxes->SetElement(0,3,neworiginXYZW[0]);
    resliceAxes->SetElement(1,3,neworiginXYZW[1]);
    resliceAxes->SetElement(2,3,neworiginXYZW[2]);
    resliceAxes->PrintSelf(std::cout, vtkIndent());
    ImageReslice->SetResliceAxes(resliceAxes.GetPointer());
    Ren->ResetCamera();
    }

  vtkISSCallback() : Plane(0), ImageReslice(0) {}

  vtkPlane* Plane;
  vtkImageReslice* ImageReslice;
  vtkRenderer* Ren;
};

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
  //rep->SetOrigin(0,0,50); //this doesn't seem to work?

  vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetInteractor(renderWindowInteractor.GetPointer());
  planeWidget->SetRepresentation(rep.GetPointer());

  // Setup the callback to do the work
  vtkNew<vtkImageReslice> reslice;
  reslice->SetInputConnection(reader->GetOutputPort());
  reslice->SetOutputDimensionality(2);
  reslice->SetOutputScalarType(VTK_UNSIGNED_CHAR);
  vtkNew<vtkISSCallback> callback;
  callback->Plane = plane.GetPointer();
  callback->ImageReslice = reslice.GetPointer();
  planeWidget->AddObserver(vtkCommand::InteractionEvent, callback.GetPointer());

  // Display the image
  vtkNew<vtkImageActor> actor;
  actor->GetMapper()->SetInputConnection(reslice->GetOutputPort());

  vtkNew<vtkRenderer> sliceRen;
  sliceRen->SetViewport(0.5, 0, 1, 1);
  sliceRen->SetBackground(0.1, 0.1, 0.0);
  sliceRen->AddActor(actor.GetPointer());
  renderWindow->AddRenderer(sliceRen.GetPointer());
  callback->Ren = sliceRen.GetPointer();

  // Enable the widget
  sliceRen->ResetCamera();
  renderer->ResetCamera();
  renderWindowInteractor->Initialize();
  planeWidget->On();
  renderWindow->Render();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
