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
#include "winviewoptsdlg.h"
#include "..\celengine\render.h"

#include "res/resource.h"

using namespace std;

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

            //Set dialog controls to reflect current label and render modes
            Dlg->SetControls(hDlg);

            //Start timer to maintain button states
            SetTimer(hDlg, 1, 500, NULL);

            return(TRUE);
        }
        break;

    case WM_COMMAND:
    {
        //Take out a Renderer* for readability sake.
        Renderer* renderer = Dlg->appCore->getRenderer();

        if(LOWORD(wParam) == IDC_SHOWATMOSPHERES)
            renderer->setRenderFlags(renderer->getRenderFlags() ^ renderer->ShowAtmospheres);
        else if(LOWORD(wParam) == IDC_SHOWCELESTIALGRID)
            renderer->setRenderFlags(renderer->getRenderFlags() ^ renderer->ShowCelestialSphere);
        else if(LOWORD(wParam) == IDC_SHOWCLOUDS)
            renderer->setRenderFlags(renderer->getRenderFlags() ^ renderer->ShowCloudMaps);
        else if(LOWORD(wParam) == IDC_SHOWCONSTELLATIONS)
            renderer->setRenderFlags(renderer->getRenderFlags() ^ renderer->ShowDiagrams);
        else if(LOWORD(wParam) == IDC_SHOWGALAXIES)
            renderer->setRenderFlags(renderer->getRenderFlags() ^ renderer->ShowGalaxies);
        else if(LOWORD(wParam) == IDC_SHOWNIGHTSIDELIGHTS)
            renderer->setRenderFlags(renderer->getRenderFlags() ^ renderer->ShowNightMaps);
        else if(LOWORD(wParam) == IDC_SHOWORBITS)
            renderer->setRenderFlags(renderer->getRenderFlags() ^ renderer->ShowOrbits);
        else if(LOWORD(wParam) == IDC_SHOWPLANETS)
            renderer->setRenderFlags(renderer->getRenderFlags() ^ renderer->ShowPlanets);
        else if(LOWORD(wParam) == IDC_SHOWSTARS)
            renderer->setRenderFlags(renderer->getRenderFlags() ^ renderer->ShowStars);
        else if(LOWORD(wParam) == IDC_LABELCONSTELLATIONS)
            renderer->setLabelMode(renderer->getLabelMode() ^ renderer->ConstellationLabels);
        else if(LOWORD(wParam) == IDC_LABELGALAXIES)
            renderer->setLabelMode(renderer->getLabelMode() ^ renderer->GalaxyLabels);
        else if(LOWORD(wParam) == IDC_LABELMAJORPLANETS)
            renderer->setLabelMode(renderer->getLabelMode() ^ renderer->MajorPlanetLabels);
        else if(LOWORD(wParam) == IDC_LABELMINORPLANETS)
            renderer->setLabelMode(renderer->getLabelMode() ^ renderer->MinorPlanetLabels);
        else if(LOWORD(wParam) == IDC_LABELSTARS)
            renderer->setLabelMode(renderer->getLabelMode() ^ renderer->StarLabels);
        else if(LOWORD(wParam) == IDC_INFOTEXT0)
            Dlg->appCore->setHudDetail(0);
        else if(LOWORD(wParam) == IDC_INFOTEXT1)
            Dlg->appCore->setHudDetail(1);
        else if(LOWORD(wParam) == IDC_INFOTEXT2)
            Dlg->appCore->setHudDetail(2);

        else if (LOWORD(wParam) == IDOK)
        {
            if (Dlg != NULL && Dlg->parent != NULL)
            {
                KillTimer(hDlg, 1);
                SendMessage(Dlg->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(Dlg));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            if (Dlg != NULL && Dlg->parent != NULL)
            {
                KillTimer(hDlg, 1);

                //Reset render flags, label mode, and hud detail to initial values
                Dlg->RestoreSettings(hDlg);

                SendMessage(Dlg->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(Dlg));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }
    case WM_TIMER:
        if((Dlg != NULL) && (Dlg->parent != NULL) && (wParam == 1))
            Dlg->SetControls(hDlg);
        break;

    case WM_DESTROY:
        if (Dlg != NULL && Dlg->parent != NULL)
        {
            SendMessage(Dlg->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(Dlg));
        }
        return TRUE;
    }

    return FALSE;
}


ViewOptionsDialog::ViewOptionsDialog(HINSTANCE appInstance,
                                   HWND _parent,
                                   CelestiaCore* _appCore) :
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
    //Read labelMode, renderFlags and hud detail
    initialRenderFlags = appCore->getRenderer()->getRenderFlags();
    initialLabelMode = appCore->getRenderer()->getLabelMode();
    initialHudDetail = appCore->getHudDetail();

    //Set checkboxes and radiobuttons
    SendDlgItemMessage(hDlg, IDC_SHOWATMOSPHERES, BM_SETCHECK,
        ((initialRenderFlags & appCore->getRenderer()->ShowAtmospheres) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCELESTIALGRID, BM_SETCHECK,
        ((initialRenderFlags & appCore->getRenderer()->ShowCelestialSphere) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCLOUDS, BM_SETCHECK,
        ((initialRenderFlags & appCore->getRenderer()->ShowCloudMaps) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCONSTELLATIONS, BM_SETCHECK,
        ((initialRenderFlags & appCore->getRenderer()->ShowDiagrams) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWGALAXIES, BM_SETCHECK,
        ((initialRenderFlags & appCore->getRenderer()->ShowGalaxies) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWNIGHTSIDELIGHTS, BM_SETCHECK,
        ((initialRenderFlags & appCore->getRenderer()->ShowNightMaps) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWORBITS, BM_SETCHECK,
        ((initialRenderFlags & appCore->getRenderer()->ShowOrbits) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWPLANETS, BM_SETCHECK,
        ((initialRenderFlags & appCore->getRenderer()->ShowPlanets) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWSTARS, BM_SETCHECK,
        ((initialRenderFlags & appCore->getRenderer()->ShowStars) != 0)? BST_CHECKED:BST_UNCHECKED, 0);

    SendDlgItemMessage(hDlg, IDC_LABELCONSTELLATIONS, BM_SETCHECK,
        ((initialLabelMode & appCore->getRenderer()->ConstellationLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELGALAXIES, BM_SETCHECK,
        ((initialLabelMode & appCore->getRenderer()->GalaxyLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELMAJORPLANETS, BM_SETCHECK,
        ((initialLabelMode & appCore->getRenderer()->MajorPlanetLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELMINORPLANETS, BM_SETCHECK,
        ((initialLabelMode & appCore->getRenderer()->MinorPlanetLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_LABELSTARS, BM_SETCHECK,
        ((initialLabelMode & appCore->getRenderer()->StarLabels) != 0)? BST_CHECKED:BST_UNCHECKED, 0);

    CheckRadioButton(hDlg, IDC_INFOTEXT0, IDC_INFOTEXT2, IDC_INFOTEXT0 + initialHudDetail);
}

void ViewOptionsDialog::RestoreSettings(HWND hDlg)
{
    appCore->getRenderer()->setRenderFlags(initialRenderFlags);
    appCore->getRenderer()->setLabelMode(initialLabelMode);
    appCore->setHudDetail(initialHudDetail);
}
