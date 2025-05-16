#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TProfile.h>
#include <TF1.h>
#include <TSystem.h>
#include <TString.h>
#include <TROOT.h>
#include <TError.h>
#include <fstream>
#include <iostream>

// Global log file stream
std::ofstream logFile("log.txt");

void log(const TString& message) {
    std::cerr << message << std::endl;
    logFile << message << std::endl;
}

// Draw and save any drawable object
void draw_and_save(TObject* obj, const TString& outpath) {
    TCanvas c("c", "c", 800, 600);
    obj->Draw();
    c.Update();
    c.SaveAs(outpath);
}

// Recursive directory traversal and plot saving
void traverse_and_save(TDirectory* dir, const TString& out_dir, const TString& rel_path = "") {
    TIter nextkey(dir->GetListOfKeys());
    TKey* key;

    while ((key = (TKey*)nextkey())) {
        TString name = key->GetName();
        TString className = key->GetClassName();

        TObject* obj = nullptr;
        try {
            obj = key->ReadObj();
        } catch (...) {
            log("ERROR: Failed to read object: " + name + " (class: " + className + ")");
            continue;
        }

        if (!obj) {
            log("WARNING: Null object encountered: " + name + " (class: " + className + ")");
            continue;
        }

        TString full_rel_path = rel_path + "/" + name;
        TString output_dir = out_dir + "/" + rel_path;
        gSystem->mkdir(output_dir, kTRUE);

        if (obj->InheritsFrom("TDirectory")) {
            traverse_and_save((TDirectory*)obj, out_dir, full_rel_path);
        }
        else if (obj->InheritsFrom("TCanvas")) {
            TCanvas* can = (TCanvas*)obj;
            can->Update();
            can->SaveAs(output_dir + "/" + name + ".png");
        }
        else if (
            obj->InheritsFrom("TH1") || obj->InheritsFrom("TH2") ||
            obj->InheritsFrom("TGraph") || obj->InheritsFrom("TMultiGraph") ||
            obj->InheritsFrom("TProfile") || obj->InheritsFrom("TF1")
        ) {
            draw_and_save(obj, output_dir + "/" + name + ".png");
        }
        else {
            log("SKIPPED: Unsupported object: " + full_rel_path + " (" + className + ")");
        }
    }
}

// Entry point function
void save_all_plots(const char* rootfile, const char* outdir) {
    gROOT->SetBatch(kTRUE); // Disable GUI windows

    TFile* file = TFile::Open(rootfile);
    if (!file || file->IsZombie()) {
        log("FATAL: Cannot open ROOT file: " + TString(rootfile));
        return;
    }

    log("INFO: Processing file: " + TString(rootfile));
    traverse_and_save(file, TString(outdir));
    file->Close();

    log("INFO: All done. Output saved to: " + TString(outdir));
    logFile.close();
}

