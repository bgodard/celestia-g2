// winviewoptsdlg.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// View Options dialog for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <windows.h>
#include <commctrl.h>
#include "winviewoptsdlg.h"
#include "..\celengine\render.h"

#include "res/resource.h"

using namespace std;

static const int DistanceSliderRange = 10000;
static const float MinDistanceLimit = 1.0f;
static const float MaxDistanceLimit = 1.0e6f;

static BOOL APIENTRY ViewOptionsProc(HWND hDlg,
                                     UINT message,
                                     UINT wParam,
                                     LONG lParam)
{
    ViewOptionsDialog* Dlg = reinterpret_cast<ViewOptionsDialog*>(GetWindowLong(hDlg, DWL_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            ViewOptionsDialog* Dlg = reinterpret_cast<ViewOptionsDialog*>(lParam);
            if (Dlg == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLong(hDlg, DWL_USER, lParam);

            //Read labelMode, renderFlags and hud detail
            Dlg->initialRenderFlags = Dlg->appCore->getRenderer()->getRenderFlags();
            Dlg->initialLabelMode = Dlg->appCore->getRenderer()->getLabelMode();
            Dlg->initialHudDetail = Dlg->appCore->getHudDetail();

            //Set dialog controls to reflect current label and render modes
            Dlg->SetControls(hDlg);

            return(TRUE);
        }
        break;

    case WM_COMMAND:
    {
        //Take out a Renderer* for readability sake.
        Renderer* renderer = Dlg->appCore->getRenderer();
        uint32 renderFlags = renderer->getRenderFlags();
        uint32 labelMode = renderer->getLabelMode();

        switch (LOWORD(wParam))
        {
        case IDC_SHOWATMOSPHERES:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowAtmospheres);
            break;
        case IDC_SHOWCELESTIALGRID:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowCelestialSphere);
            break;
        case IDC_SHOWCLOUDS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowCloudMaps);
            break;
        case IDC_SHOWCONSTELLATIONS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowDiagrams);
            break;
        case IDC_SHOWECLIPSESHADOWS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowEclipseShadows);
            break;
        case IDC_SHOWGALAXIES:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowGalaxies);
            break;
        case IDC_SHOWNIGHTSIDELIGHTS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowNightMaps);
            break;
        case IDC_SHOWORBITS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowOrbits);
            break;
        case IDC_SHOWPLANETS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowPlanets);
            break;
        case IDC_SHOWSTARS:
            {
                renderer->setRenderFlags(renderFlags ^ Renderer::ShowStars);

                HWND hAsPoints = GetDlgItem(hDlg, IDC_SHOWSTARSASPOINTS);
                if (hAsPoints)
                    EnableWindow(hAsPoints, (BOOL)(renderFlags & Renderer::ShowStars));
            }
            break;
        case IDC_SHOWSTARSASPOINTS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowStarsAsPoints);
            break;
        case IDC_SHOWCONSTELLATIONBORDERS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowBoundaries);
            break;
        case IDC_SHOWRINGSHADOWS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowRingShadows);
            break;
        case IDC_SHOWCOMETTAILS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowCometTails);
            break;
        case IDC_SHOWMARKERS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowMarkers);
            break;
        case IDC_LABELCONSTELLATIONS:
            renderer->setLabelMode(labelMode ^ Renderer::ConstellationLabels);
            break;
        case IDC_LABELGALAXIES:
            renderer->setLabelMode(labelMode ^ Renderer::GalaxyLabels);
            break;
        case IDC_LABELPLANETS:
            renderer->setLabelMode(labelMode ^ Renderer::PlanetLabels);
            break;
        case IDC_LABELMOONS:
            renderer->setLabelMode(labelMode ^ Renderer::MoonLabels);
            break;
        case IDC_LABELSTARS:
            renderer->setLabelMode(labelMode ^ Renderer::StarLabels);
            break;
        case IDC_LABELASTEROIDS:
            renderer->setLabelMode(labelMode ^ Renderer::AsteroidLabels);
            break;
        case IDC_LABELSPACECRAFT:
            renderer->setLabelMode(labelMode ^ Renderer::SpacecraftLabels);
            break;
        case IDC_INFOTEXT0:
            Dlg->appCore->setHudDetail(0);
            break;
        case IDC_INFOTEXT1:
            Dlg->appCore->setHudDetail(1);
            break;
        case IDC_INFOTEXT2:
            Dlg->appCore->setHudDetail(2);
            break;
        case IDOK:
            if (Dlg != NULL && Dlg->parent != NULL)
            {
                SendMessage(Dlg->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(Dlg));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        case IDCANCEL:
            if (Dlg != NULL && Dlg->parent != NULL)
            {
                // Reset render flags, label mode, and hud detail to
                // initial values
                Dlg->RestoreSettings(hDlg);
                SendMessage(Dlg->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(Dlg));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    case WM_DESTROY:
        if (Dlg != NULL && Dlg->parent != NULL)
        {
            SendMessage(Dlg->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(Dlg));
        }
        return TRUE;

    case WM_HSCROLL:
        {
            WORD sbValue = LOWORD(wParam);
            switch (sbValue)
            {
            case SB_THUMBTRACK:
                // case SB_ENDSCROLL:
                {
                    char val[16];
                    HWND hwnd = GetDlgItem(hDlg, IDC_EDIT_FILTER_DISTANCE);
                    float logDistanceLimit = (float) HIWORD(wParam) /
                        (float) DistanceSliderRange;
                    float distanceLimit = (float) pow(MaxDistanceLimit, 
                                                 logDistanceLimit);
                    sprintf(val, "%d", (int) distanceLimit);
                    SetWindowText(hwnd, val);
                    Dlg->appCore->getRenderer()->setDistanceLimit(distanceLimit);
                    break;
                }

            default:
                cout << "Slider msg: " << sbValue << '\n';
                break;
#if 0
            case SB_THUMBPOSITION:
                {
                    browser->nStars = (int)HIWORD(wParam);
                    RefreshItems(hDlg, browser);
                    break;
                }
#endif
            }
        }
    }

    return FALSE;
}


ViewOptionsDialog::ViewOptionsDialog(HINSTANCE appInstance,
                                     HWND _parent,
                                     CelestiaCore* _appCore) :
    CelestiaWatcher(*_appCore),
    appCore(_appCore),
    parent(_parent)
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_VIEWOPTIONS),
                             parent,
                             ViewOptionsProc,
                             reinterpret_cast<LONG>(this));
}

void ViewOptionsDialog::SetControls(HWND hDlg)
{
    int renderFlags = appCore->getRenderer()->getRenderFlags();
    int labelMode = appCore->getRenderer()->getLabelMode();
    int hudDetail = appCore->getHudDetail();

    //Set checkboxes and radiobuttons
    SendDlgItemMessage(hDlg, IDC_SHOWATMOSPHERES, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowAtmospheres) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCELESTIALGRID, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowCelestialSphere) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCLOUDS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowCloudMaps) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCONSTELLATIONS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowDiagrams) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWECLIPSESHADOWS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowEclipseShadows) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWGALAXIES, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowGalaxies) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWNIGHTSIDELIGHTS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowNightMaps) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWORBITS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowOrbits) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWPLANETS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowPlanets) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWSTARS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowStars) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWSTARSASPOINTS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowStarsAsPoints) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCONSTELLATIONBORDERS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowBoundaries) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWRINGSHADOWS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowRingShadows) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCOMETTAILS, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowCometTails) != 0)? BST_CHECKED:BST_UNCHECKED, 0);

    SendDlgItemMessage(hDlg, IDC_LABELCONSTELLATIONS, BM_SETCHECK,
        ((labelMode & appCore->getRenderer()->ConstellationLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELGALAXIES, BM_SETCHECK,
        ((labelMode & appCore->getRenderer()->GalaxyLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELPLANETS, BM_SETCHECK,
        ((labelMode & appCore->getRenderer()->PlanetLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELMOONS, BM_SETCHECK,
        ((labelMode & appCore->getRenderer()->MoonLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELSTARS, BM_SETCHECK,
        ((labelMode & appCore->getRenderer()->StarLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELASTEROIDS, BM_SETCHECK,
        ((labelMode & appCore->getRenderer()->AsteroidLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELSPACECRAFT, BM_SETCHECK,
        ((labelMode & appCore->getRenderer()->SpacecraftLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);

    CheckRadioButton(hDlg, IDC_INFOTEXT0, IDC_INFOTEXT2, IDC_INFOTEXT0 + hudDetail);

    // Set up distance slider
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FILTER_DISTANCE,
                       TBM_SETRANGE,
                       (WPARAM)TRUE,
                       (LPARAM) MAKELONG(0, DistanceSliderRange));
    float distanceLimit = appCore->getRenderer()->getDistanceLimit();
    float logDistanceLimit = (float) (log(distanceLimit) / 
                                      log(MaxDistanceLimit));
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FILTER_DISTANCE,
                       TBM_SETPOS,
                       (WPARAM) TRUE,
                       (LPARAM) (logDistanceLimit * DistanceSliderRange));

    char val[16];
    HWND hwnd = GetDlgItem(hDlg, IDC_EDIT_FILTER_DISTANCE);
    sprintf(val, "%d", (int) distanceLimit);
    SetWindowText(hwnd, val);
}


void ViewOptionsDialog::RestoreSettings(HWND hDlg)
{
    appCore->getRenderer()->setRenderFlags(initialRenderFlags);
    appCore->getRenderer()->setLabelMode(initialLabelMode);
    appCore->setHudDetail(initialHudDetail);
}

void ViewOptionsDialog::notifyChange(CelestiaCore*, int)
{
    if (parent != NULL)
        SetControls(hwnd);
}
