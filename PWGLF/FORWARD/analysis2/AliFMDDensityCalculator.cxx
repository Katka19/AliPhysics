// This class calculates the inclusive charged particle density
// in each for the 5 FMD rings. 
//
#include "AliFMDDensityCalculator.h"
#include <AliESDFMD.h>
#include <TAxis.h>
#include <TList.h>
#include <TMath.h>
#include "AliForwardCorrectionManager.h"
#include "AliFMDCorrDoubleHit.h"
#include "AliFMDCorrELossFit.h"
#include "AliLog.h"
#include <TH2D.h>
#include <TProfile.h>
#include <THStack.h>
#include <TROOT.h>
#include <TParameter.h>
#include <iostream>
#include <iomanip>

ClassImp(AliFMDDensityCalculator)
#if 0
; // For Emacs
#endif 

//____________________________________________________________________
AliFMDDensityCalculator::AliFMDDensityCalculator()
  : TNamed(), 
    fRingHistos(),
    fSumOfWeights(0),
    fWeightedSum(0),
    fCorrections(0),
    fMaxParticles(5),
    fUsePoisson(false),
    fUsePhiAcceptance(kPhiCorrectNch),
    fAccI(0),
    fAccO(0),
    fFMD1iMax(0),
    fFMD2iMax(0),
    fFMD2oMax(0),
    fFMD3iMax(0),
    fFMD3oMax(0),
    fMaxWeights(0),
    fLowCuts(0),
    fEtaLumping(32), 
    fPhiLumping(4),    
    fDebug(0),
    fCuts(),
    fRecalculateEta(false)
{
  // 
  // Constructor 
  //
  DGUARD(fDebug, 0, "Default CTOR of FMD density calculator");
}

//____________________________________________________________________
AliFMDDensityCalculator::AliFMDDensityCalculator(const char* title)
  : TNamed("fmdDensityCalculator", title), 
    fRingHistos(), 
    fSumOfWeights(0),
    fWeightedSum(0),
    fCorrections(0),
    fMaxParticles(5),
    fUsePoisson(false),
    fUsePhiAcceptance(kPhiCorrectNch),
    fAccI(0),
    fAccO(0),
    fFMD1iMax(0),
    fFMD2iMax(0),
    fFMD2oMax(0),
    fFMD3iMax(0),
    fFMD3oMax(0),
    fMaxWeights(0),
    fLowCuts(0),
    fEtaLumping(32), 
    fPhiLumping(4),
    fDebug(0),
    fCuts(),
    fRecalculateEta(false)
{
  // 
  // Constructor 
  // 
  // Parameters:
  //    name Name of object
  //
  DGUARD(fDebug, 0, "Named CTOR of FMD density calculator: %s", title);
  fRingHistos.SetName(GetName());
  fRingHistos.SetOwner();
  fRingHistos.Add(new RingHistos(1, 'I'));
  fRingHistos.Add(new RingHistos(2, 'I'));
  fRingHistos.Add(new RingHistos(2, 'O'));
  fRingHistos.Add(new RingHistos(3, 'I'));
  fRingHistos.Add(new RingHistos(3, 'O'));
  fSumOfWeights = new TH1D("sumOfWeights", "Sum of Landau weights",
			   200, 0, 20);
  fSumOfWeights->SetFillColor(kRed+1);
  fSumOfWeights->SetXTitle("#sum_{i} a_{i} f_{i}(#Delta)");
  fWeightedSum  = new TH1D("weightedSum", "Weighted sum of Landau propability",
			   200, 0, 20);
  fWeightedSum->SetFillColor(kBlue+1);
  fWeightedSum->SetXTitle("#sum_{i} i a_{i} f_{i}(#Delta)");
  fCorrections  = new TH1D("corrections", "Distribution of corrections", 
			   100, 0, 10);
  fCorrections->SetFillColor(kBlue+1);
  fCorrections->SetXTitle("correction");

  fAccI = GenerateAcceptanceCorrection('I');
  fAccO = GenerateAcceptanceCorrection('O');

  fMaxWeights = new TH2D("maxWeights", "Maximum i of a_{i}'s to use", 
			 1, 0, 1, 1, 0, 1);
  fMaxWeights->SetXTitle("#eta");
  fMaxWeights->SetDirectory(0);

  fLowCuts = new TH2D("lowCuts", "Low cuts used", 1, 0, 1, 1, 0, 1);
  fLowCuts->SetXTitle("#eta");
  fLowCuts->SetDirectory(0);

}

//____________________________________________________________________
AliFMDDensityCalculator::AliFMDDensityCalculator(const 
						 AliFMDDensityCalculator& o)
  : TNamed(o), 
    fRingHistos(), 
    fSumOfWeights(o.fSumOfWeights),
    fWeightedSum(o.fWeightedSum),
    fCorrections(o.fCorrections),
    fMaxParticles(o.fMaxParticles),
    fUsePoisson(o.fUsePoisson),
    fUsePhiAcceptance(o.fUsePhiAcceptance),
    fAccI(o.fAccI),
    fAccO(o.fAccO),
    fFMD1iMax(o.fFMD1iMax),
    fFMD2iMax(o.fFMD2iMax),
    fFMD2oMax(o.fFMD2oMax),
    fFMD3iMax(o.fFMD3iMax),
    fFMD3oMax(o.fFMD3oMax),
    fMaxWeights(o.fMaxWeights),
    fLowCuts(o.fLowCuts),
    fEtaLumping(o.fEtaLumping), 
    fPhiLumping(o.fPhiLumping),
    fDebug(o.fDebug),
    fCuts(o.fCuts),
    fRecalculateEta(o.fRecalculateEta)
{
  // 
  // Copy constructor 
  // 
  // Parameters:
  //    o Object to copy from 
  //
  DGUARD(fDebug, 0, "Copy CTOR of FMD density calculator");
  TIter    next(&o.fRingHistos);
  TObject* obj = 0;
  while ((obj = next())) fRingHistos.Add(obj);
}

//____________________________________________________________________
AliFMDDensityCalculator::~AliFMDDensityCalculator()
{
  // 
  // Destructor 
  //
  DGUARD(fDebug, 3, "DTOR of FMD density calculator");
  fRingHistos.Delete();
}

//____________________________________________________________________
AliFMDDensityCalculator&
AliFMDDensityCalculator::operator=(const AliFMDDensityCalculator& o)
{
  // 
  // Assignement operator
  // 
  // Parameters:
  //    o Object to assign from 
  // 
  // Return:
  //    Reference to this object
  //
  DGUARD(fDebug, 3, "Assignment of FMD density calculator");
  if (&o == this) return *this; 
  TNamed::operator=(o);

  fDebug              = o.fDebug;
  fMaxParticles       = o.fMaxParticles;
  fUsePoisson         = o.fUsePoisson;
  fUsePhiAcceptance   = o.fUsePhiAcceptance;
  fAccI               = o.fAccI;
  fAccO               = o.fAccO;
  fFMD1iMax           = o.fFMD1iMax;
  fFMD2iMax           = o.fFMD2iMax;
  fFMD2oMax           = o.fFMD2oMax;
  fFMD3iMax           = o.fFMD3iMax;
  fFMD3oMax           = o.fFMD3oMax;
  fMaxWeights         = o.fMaxWeights;
  fLowCuts            = o.fLowCuts;
  fEtaLumping         = o.fEtaLumping;
  fPhiLumping         = o.fPhiLumping;
  fCuts               = o.fCuts;
  fRecalculateEta     = o.fRecalculateEta;

  fRingHistos.Delete();
  TIter    next(&o.fRingHistos);
  TObject* obj = 0;
  while ((obj = next())) fRingHistos.Add(obj);
  
  return *this;
}

//____________________________________________________________________
void
AliFMDDensityCalculator::Init(const TAxis& axis)
{
  // Intialize this sub-algorithm 
  //
  // Parameters:
  //   etaAxis   Not used
  DGUARD(fDebug, 1, "Initialize FMD density calculator");
  CacheMaxWeights(axis);
 
  TIter    next(&fRingHistos);
  RingHistos* o = 0;
  while ((o = static_cast<RingHistos*>(next()))) {
    o->Init(axis);
    // o->fMultCut = fCuts.GetFixedCut(o->fDet, o->fRing);
    // o->fPoisson.Init(o->fDet,o->fRing,fEtaLumping, fPhiLumping);
  }
}

//____________________________________________________________________
AliFMDDensityCalculator::RingHistos*
AliFMDDensityCalculator::GetRingHistos(UShort_t d, Char_t r) const
{
  // 
  // Get the ring histogram container 
  // 
  // Parameters:
  //    d Detector
  //    r Ring 
  // 
  // Return:
  //    Ring histogram container 
  //
  Int_t idx = -1;
  switch (d) { 
  case 1: idx = 0; break;
  case 2: idx = 1 + (r == 'I' || r == 'i' ? 0 : 1); break;
  case 3: idx = 3 + (r == 'I' || r == 'i' ? 0 : 1); break;
  }
  if (idx < 0 || idx >= fRingHistos.GetEntries()) {
    AliWarning(Form("Index %d of FMD%d%c out of range", idx, d, r));
    return 0;
  }
  
  return static_cast<RingHistos*>(fRingHistos.At(idx));
}

//____________________________________________________________________
Double_t
AliFMDDensityCalculator::GetMultCut(UShort_t d, Char_t r, Int_t ieta,
				    Bool_t errors) const
{
  // 
  // Get the multiplicity cut.  If the user has set fMultCut (via
  // SetMultCut) then that value is used.  If not, then the lower
  // value of the fit range for the enery loss fits is returned.
  // 
  // Return:
  //    Lower cut on multiplicity
  //
  return fCuts.GetMultCut(d,r,ieta,errors);
}
    
//____________________________________________________________________
Double_t
AliFMDDensityCalculator::GetMultCut(UShort_t d, Char_t r, Double_t eta,
				    Bool_t errors) const
{
  // 
  // Get the multiplicity cut.  If the user has set fMultCut (via
  // SetMultCut) then that value is used.  If not, then the lower
  // value of the fit range for the enery loss fits is returned.
  // 
  // Return:
  //    Lower cut on multiplicity
  //
  return fCuts.GetMultCut(d,r,eta,errors);
}
  
//____________________________________________________________________
Bool_t
AliFMDDensityCalculator::Calculate(const AliESDFMD&        fmd,
				   AliForwardUtil::Histos& hists,
				   UShort_t                vtxbin, 
				   Bool_t                  lowFlux,
				   Double_t                /*cent*/, 
				   Double_t                zvtx)
{
  // 
  // Do the calculations 
  // 
  // Parameters:
  //    fmd      AliESDFMD object (possibly) corrected for sharing
  //    hists    Histogram cache
  //    vtxBin   Vertex bin 
  //    lowFlux  Low flux flag. 
  // 
  // Return:
  //    true on successs 
  DGUARD(fDebug, 1, "Calculate density in FMD density calculator");
  
  for (UShort_t d=1; d<=3; d++) { 
    UShort_t nr = (d == 1 ? 1 : 2);
    for (UShort_t q=0; q<nr; q++) { 
      Char_t      r = (q == 0 ? 'I' : 'O');
      UShort_t    ns= (q == 0 ?  20 :  40);
      UShort_t    nt= (q == 0 ? 512 : 256);
      TH2D*       h = hists.Get(d,r);
      RingHistos* rh= GetRingHistos(d,r);
      if (!rh) { 
	AliError(Form("No ring histogram found for FMD%d%c", d, r));
	fRingHistos.ls();
	return false;
      }
      // rh->fPoisson.SetObject(d,r,vtxbin,cent);
      rh->fPoisson.Reset(0);
      // rh->ResetPoissonHistos(h, fEtaLumping, fPhiLumping);
      
      for (UShort_t s=0; s<ns; s++) { 
	for (UShort_t t=0; t<nt; t++) {
	  
	  Float_t  mult = fmd.Multiplicity(d,r,s,t);
	  Float_t  phi  = fmd.Phi(d,r,s,t) / 180 * TMath::Pi();
	  Float_t  eta  = fmd.Eta(d,r,s,t);
	 
	  
	  if(fRecalculateEta)  
	    eta = AliForwardUtil::GetEtaFromStrip(d,r,s,t,zvtx);
	  
	  if (mult == AliESDFMD::kInvalidMult || mult > 20) {
	    rh->fPoisson.Fill(t , s, false);
	    rh->fEvsM->Fill(mult,0);
	    continue;
	  }
	  if (fUsePhiAcceptance == kPhiCorrectELoss) 
	    mult *= AcceptanceCorrection(r,t);

	  Double_t cut  = 1024;
	  if (eta != AliESDFMD::kInvalidEta) cut = GetMultCut(d, r, eta,false);

	  Double_t n   = 0;
	  if (cut > 0 && mult > cut) 
	    n = NParticles(mult,d,r,s,t,vtxbin,eta,lowFlux);
	  
	  rh->fELoss->Fill(mult);
	  rh->fEvsN->Fill(mult,n);
	  rh->fEtaVsN->Fill(eta, n);
	  
	  Double_t c = Correction(d,r,s,t,vtxbin,eta,lowFlux);
	  fCorrections->Fill(c);
	  if (c > 0) n /= c;
	  rh->fEvsM->Fill(mult,n);
	  rh->fEtaVsM->Fill(eta, n);
	  rh->fCorr->Fill(eta, c);

	  Bool_t hit = (n > 0.9 && c > 0);
	  if (hit) rh->fELossUsed->Fill(mult);
	  rh->fPoisson.Fill(t,s,hit,1./c);
	  h->Fill(eta,phi,n);
	  if (!fUsePoisson) rh->fDensity->Fill(eta,phi,n);
	} // for t
      } // for s 
      
      TH2D* hclone = static_cast<TH2D*>(h->Clone("hclone"));
      if (!fUsePoisson) hclone->Reset();
      if ( fUsePoisson) h->Reset();
      
      TH2D* poisson = rh->fPoisson.Result();
      for (Int_t t=0; t < poisson->GetNbinsX(); t++) { 
	for (Int_t s=0; s < poisson->GetNbinsY(); s++) { 
	  
	  Double_t poissonV = poisson->GetBinContent(t+1,s+1);
	  Double_t  phi  = fmd.Phi(d,r,s,t) / 180 * TMath::Pi();
	  Double_t  eta  = fmd.Eta(d,r,s,t);
	  if(fRecalculateEta)  
	    eta = AliForwardUtil::GetEtaFromStrip(d,r,s,t,zvtx);
	  if (fUsePoisson)
	    h->Fill(eta,phi,poissonV);
	  else
	    hclone->Fill(eta,phi,poissonV);
	  if (fUsePoisson) rh->fDensity->Fill(eta, phi, poissonV);
	}
      }
      
      for (Int_t ieta=1; ieta <= h->GetNbinsX(); ieta++) { 
	for (Int_t iphi=1; iphi<= h->GetNbinsY(); iphi++) { 
	  
	  Double_t poissonV =  0; //h->GetBinContent(,s+1);
	  Double_t eLossV =  0;
	  if(fUsePoisson) { 
	    poissonV = h->GetBinContent(ieta,iphi);
	    eLossV  = hclone->GetBinContent(ieta,iphi);
	  }
	  else { 
	    poissonV = hclone->GetBinContent(ieta,iphi);
	    eLossV  = h->GetBinContent(ieta,iphi);
	  }
	  
	  rh->fELossVsPoisson->Fill(eLossV, poissonV);
	}
      }
      delete hclone;
      
    } // for q
  } // for d

  return kTRUE;
}

//_____________________________________________________________________
Int_t
AliFMDDensityCalculator::FindMaxWeight(const AliFMDCorrELossFit* cor,
				       UShort_t d, Char_t r, Int_t iEta) const
{
  // 
  // Find the max weight to use for FMD<i>dr</i> in eta bin @a iEta
  // 
  // Parameters:
  //    cor   Correction
  //    d     Detector 
  //    r     Ring 
  //    iEta  Eta bin 
  //
  DGUARD(fDebug, 3, "Find maximum weight in FMD density calculator");
  if(!cor) return -1; 

  AliFMDCorrELossFit::ELossFit* fit = cor->GetFit(d,r,iEta);
  if (!fit) { 
    // AliWarning(Form("No energy loss fit for FMD%d%c at eta=%f", d, r, eta));
    return -1;
  }
  return TMath::Min(Int_t(fMaxParticles), fit->FindMaxWeight());
}
  
//_____________________________________________________________________
void
AliFMDDensityCalculator::CacheMaxWeights(const TAxis& axis)
{
  // 
  // Find the max weights and cache them 
  // 
  DGUARD(fDebug, 2, "Cache maximum weights in FMD density calculator");
  AliForwardCorrectionManager&  fcm = AliForwardCorrectionManager::Instance();
  AliFMDCorrELossFit*           cor = fcm.GetELossFit();
  TAxis eta(axis); // Set to default from parent task 
  if(cor) 
    eta.Set(cor->GetEtaAxis().GetNbins(), 
	    cor->GetEtaAxis().GetXmin(), 
	    cor->GetEtaAxis().GetXmax());

  Int_t nEta = eta.GetNbins();
  fFMD1iMax.Set(nEta);
  fFMD2iMax.Set(nEta);
  fFMD2oMax.Set(nEta);
  fFMD3iMax.Set(nEta);
  fFMD3oMax.Set(nEta);
  
  fMaxWeights->SetBins(nEta, eta.GetXmin(), eta.GetXmax(), 5, .5, 5.5);
  fMaxWeights->GetYaxis()->SetBinLabel(1, "FMD1i");
  fMaxWeights->GetYaxis()->SetBinLabel(2, "FMD2i");
  fMaxWeights->GetYaxis()->SetBinLabel(3, "FMD2o");
  fMaxWeights->GetYaxis()->SetBinLabel(4, "FMD3i");
  fMaxWeights->GetYaxis()->SetBinLabel(5, "FMD3o");

  AliInfo(Form("Get eta axis with %d bins from %f to %f",
	       nEta, eta.GetXmin(), eta.GetXmax()));
  fLowCuts->SetBins(nEta, eta.GetXmin(), eta.GetXmax(), 5, .5, 5.5);
  fLowCuts->GetYaxis()->SetBinLabel(1, "FMD1i");
  fLowCuts->GetYaxis()->SetBinLabel(2, "FMD2i");
  fLowCuts->GetYaxis()->SetBinLabel(3, "FMD2o");
  fLowCuts->GetYaxis()->SetBinLabel(4, "FMD3i");
  fLowCuts->GetYaxis()->SetBinLabel(5, "FMD3o");
  
  for (Int_t i = 0; i < nEta; i++) {
    Double_t w[5];
    w[0] = fFMD1iMax[i] = FindMaxWeight(cor, 1, 'I', i+1);
    w[1] = fFMD2iMax[i] = FindMaxWeight(cor, 2, 'I', i+1);
    w[2] = fFMD2oMax[i] = FindMaxWeight(cor, 2, 'O', i+1);
    w[3] = fFMD3iMax[i] = FindMaxWeight(cor, 3, 'I', i+1);
    w[4] = fFMD3oMax[i] = FindMaxWeight(cor, 3, 'O', i+1);
    Double_t l[5];
    l[0] = GetMultCut(1, 'I', i+1, false);
    l[1] = GetMultCut(2, 'I', i+1, false);
    l[2] = GetMultCut(2, 'O', i+1, false);
    l[3] = GetMultCut(3, 'I', i+1, false);
    l[4] = GetMultCut(3, 'O', i+1, false);
    for (Int_t j = 0; j < 5; j++) { 
      if (w[j] > 0) fMaxWeights->SetBinContent(i+1, j+1, w[j]);
      if (l[j] > 0) fLowCuts->SetBinContent(i+1, j+1, l[j]);
    }
  }
}

//_____________________________________________________________________
Int_t
AliFMDDensityCalculator::GetMaxWeight(UShort_t d, Char_t r, Int_t iEta) const
{
  // 
  // Find the (cached) maximum weight for FMD<i>dr</i> in 
  // @f$\eta@f$ bin @a iEta
  // 
  // Parameters:
  //    d     Detector
  //    r     Ring
  //    iEta  Eta bin
  // 
  // Return:
  //    max weight or <= 0 in case of problems 
  //
  if (iEta < 0) return -1;

  const TArrayI* max  = 0;
  switch (d) { 
  case 1:  max = &fFMD1iMax;                                       break;
  case 2:  max = (r == 'I' || r == 'i' ? &fFMD2iMax : &fFMD2oMax); break;
  case 3:  max = (r == 'I' || r == 'i' ? &fFMD3iMax : &fFMD3oMax); break;
  }
  if (!max) { 
    AliWarning(Form("No array for FMD%d%c", d, r));
    return -1;
  }
  
  if (iEta >= max->fN) { 
    AliWarning(Form("Eta bin %3d out of bounds [0,%d]", 
		    iEta, max->fN-1));
    return -1;
  }

  AliDebug(30,Form("Max weight for FMD%d%c eta bin %3d: %d", d, r, iEta, 
		   max->At(iEta)));
  return max->At(iEta);
}

//_____________________________________________________________________
Int_t
AliFMDDensityCalculator::GetMaxWeight(UShort_t d, Char_t r, Float_t eta) const
{
  // 
  // Find the (cached) maximum weight for FMD<i>dr</i> iat
  // @f$\eta@f$ 
  // 
  // Parameters:
  //    d     Detector
  //    r     Ring
  //    eta   Eta bin
  // 
  // Return:
  //    max weight or <= 0 in case of problems 
  //
  AliForwardCorrectionManager&  fcm  = AliForwardCorrectionManager::Instance();
  Int_t                         iEta = fcm.GetELossFit()->FindEtaBin(eta) -1;

  return GetMaxWeight(d, r, iEta);
}

//_____________________________________________________________________
Float_t 
AliFMDDensityCalculator::NParticles(Float_t  mult, 
				    UShort_t d, 
				    Char_t   r, 
				    UShort_t /*s*/, 
				    UShort_t /*t*/, 
				    UShort_t /*v*/, 
				    Float_t  eta,
				    Bool_t   lowFlux) const
{
  // 
  // Get the number of particles corresponding to the signal mult
  // 
  // Parameters:
  //    mult     Signal
  //    d        Detector
  //    r        Ring 
  //    s        Sector 
  //    t        Strip (not used)
  //    v        Vertex bin 
  //    eta      Pseudo-rapidity 
  //    lowFlux  Low-flux flag 
  // 
  // Return:
  //    The number of particles 
  //
  // if (mult <= GetMultCut()) return 0;
  DGUARD(fDebug, 3, "Calculate Nch in FMD density calculator");
  if (lowFlux) return 1;
  
  AliForwardCorrectionManager&  fcm = AliForwardCorrectionManager::Instance();
  AliFMDCorrELossFit::ELossFit* fit = fcm.GetELossFit()->FindFit(d,r,eta);
  if (!fit) { 
    AliWarning(Form("No energy loss fit for FMD%d%c at eta=%f", d, r, eta));
    return 0;
  }
  
  Int_t    m   = GetMaxWeight(d,r,eta); // fit->FindMaxWeight();
  if (m < 1) { 
    AliWarning(Form("No good fits for FMD%d%c at eta=%f", d, r, eta));
    return 0;
  }
  
  UShort_t n   = TMath::Min(fMaxParticles, UShort_t(m));
  Double_t ret = fit->EvaluateWeighted(mult, n);
  
  if (fDebug > 10) {
    AliInfo(Form("FMD%d%c, eta=%7.4f, %8.5f -> %8.5f", d, r, eta, mult, ret));
  }
    
  fWeightedSum->Fill(ret);
  fSumOfWeights->Fill(ret);
  
  return ret;
}

//_____________________________________________________________________
Float_t 
AliFMDDensityCalculator::Correction(UShort_t d, 
				    Char_t   r, 
				    UShort_t /*s*/, 
				    UShort_t t, 
				    UShort_t /*v*/, 
				    Float_t  eta,
				    Bool_t   lowFlux) const
{
  // 
  // Get the inverse correction factor.  This consist of
  // 
  // - acceptance correction (corners of sensors) 
  // - double hit correction (for low-flux events) 
  // - dead strip correction 
  // 
  // Parameters:
  //    d        Detector
  //    r        Ring 
  //    s        Sector 
  //    t        Strip (not used)
  //    v        Vertex bin 
  //    eta      Pseudo-rapidity 
  //    lowFlux  Low-flux flag 
  // 
  // Return:
  //    
  //
  DGUARD(fDebug, 3, "Apply correction in FMD density calculator");
  AliForwardCorrectionManager&  fcm = AliForwardCorrectionManager::Instance();

  Float_t correction = 1; 
  if (fUsePhiAcceptance == kPhiCorrectNch) 
    correction = AcceptanceCorrection(r,t);
  if (lowFlux) { 
    TH1D* dblHitCor = 0;
    if (fcm.GetDoubleHit()) 
      dblHitCor = fcm.GetDoubleHit()->GetCorrection(d,r);

    if (dblHitCor) {
      Double_t dblC = dblHitCor->GetBinContent(dblHitCor->FindBin(eta));
      if (dblC > 0) correction *= dblC;
    }
    else {
      AliWarning(Form("Missing double hit correction for FMD%d%c",d,r));
    }
  }
  return correction;
}

//_____________________________________________________________________
TH1D*
AliFMDDensityCalculator::GenerateAcceptanceCorrection(Char_t r) const
{
  // 
  // Generate the acceptance corrections 
  // 
  // Parameters:
  //    r Ring to generate for 
  // 
  // Return:
  //    Newly allocated histogram of acceptance corrections
  //
  DGUARD(fDebug, 3, "Make acceptance correction in FMD density calculator");
  const Double_t ic1[] = { 4.9895, 15.3560 };
  const Double_t ic2[] = { 1.8007, 17.2000 };
  const Double_t oc1[] = { 4.2231, 26.6638 };
  const Double_t oc2[] = { 1.8357, 27.9500 };
  const Double_t* c1   = (r == 'I' || r == 'i' ? ic1      : oc1);
  const Double_t* c2   = (r == 'I' || r == 'i' ? ic2      : oc2);
  Double_t  minR       = (r == 'I' || r == 'i' ?   4.5213 :  15.4);
  Double_t  maxR       = (r == 'I' || r == 'i' ?  17.2    :  28.0);
  Int_t     nStrips    = (r == 'I' || r == 'i' ? 512      : 256);
  Int_t     nSec       = (r == 'I' || r == 'i' ?  20      :  40);
  Float_t   basearc    = 2 * TMath::Pi() / nSec;
  Double_t  rad        = maxR - minR;
  Float_t   segment    = rad / nStrips;
  Float_t   cr         = TMath::Sqrt(c1[0]*c1[0]+c1[1]*c1[1]);

  // Numbers used to find end-point of strip.
  // (See http://mathworld.wolfram.com/Circle-LineIntersection.html)
  Float_t D            = c1[0] * c2[1] - c1[1] * c2[0];
  Float_t dx           = c2[0] - c1[0];
  Float_t dy           = c2[1] - c1[1];
  Float_t dr           = TMath::Sqrt(dx*dx+dy*dy);

  TH1D* ret = new TH1D(Form("acc%c", r), 
		       Form("Acceptance correction for FMDx%c", r), 
		       nStrips, -.5, nStrips-.5);
  ret->SetXTitle("Strip");
  ret->SetYTitle("#varphi acceptance");
  ret->SetDirectory(0);
  ret->SetFillColor(r == 'I' || r == 'i' ? kRed+1 : kBlue+1);
  ret->SetFillStyle(3001);

  for (Int_t t = 0; t < nStrips; t++) { 
    Float_t   radius     = minR + t * segment;
    
    // If the radius of the strip is smaller than the radius corresponding 
    // to the first corner we have a full strip length 
    if (radius <= cr) {
      ret->SetBinContent(t+1, 1);
      continue;
    }

    // Next, we should find the end-point of the strip - that is, 
    // the coordinates where the line from c1 to c2 intersects a circle 
    // with radius given by the strip. 
    // (See http://mathworld.wolfram.com/Circle-LineIntersection.html)
    // Calculate the determinant 
    Float_t det = radius * radius * dr * dr - D*D;

    if (det <=  0) { 
      // <0 means No intersection
      // =0 means Exactly tangent
      ret->SetBinContent(t+1, 1);
      continue;
    }

    // Calculate end-point and the corresponding opening angle 
    Float_t x   = (+D * dy + dx * TMath::Sqrt(det)) / dr / dr;
    Float_t y   = (-D * dx + dy * TMath::Sqrt(det)) / dr / dr;
    Float_t th  = TMath::ATan2(x, y);

    ret->SetBinContent(t+1, th / basearc);
  }
  return ret;
}

//_____________________________________________________________________
Float_t 
AliFMDDensityCalculator::AcceptanceCorrection(Char_t r, UShort_t t) const
{
  // 
  // Get the acceptance correction for strip @a t in an ring of type @a r
  // 
  // Parameters:
  //    r  Ring type ('I' or 'O')
  //    t  Strip number 
  // 
  // Return:
  //    Inverse acceptance correction 
  //
  TH1D* acc = (r == 'I' || r == 'i' ? fAccI : fAccO);
  return acc->GetBinContent(t+1);
}

//____________________________________________________________________
void
AliFMDDensityCalculator::ScaleHistograms(const TList* dir, Int_t nEvents)
{
  // 
  // Scale the histograms to the total number of events 
  // 
  // Parameters:
  //    dir     where to put the output
  //    nEvents Number of events 
  //
  DGUARD(fDebug, 1, "Scale histograms in FMD density calculator");
  if (nEvents <= 0) return;
  TList* d = static_cast<TList*>(dir->FindObject(GetName()));
  if (!d) return;

  TIter    next(&fRingHistos);
  RingHistos* o = 0;
  THStack* sums = new THStack("sums", "sums of ring signals");
  while ((o = static_cast<RingHistos*>(next()))) {
    o->ScaleHistograms(d, nEvents);
    TH1D* sum = o->fDensity->ProjectionX(o->GetName(), 1, 
					 o->fDensity->GetNbinsY(),"e");
    sum->Scale(1., "width");
    sum->SetTitle(o->GetName());
    sum->SetDirectory(0);
    sum->SetYTitle("#sum N_{ch,incl}");
    sums->Add(sum);
  }
  d->Add(sums);
}

//____________________________________________________________________
void
AliFMDDensityCalculator::DefineOutput(TList* dir)
{
  // 
  // Output diagnostic histograms to directory 
  // 
  // Parameters:
  //    dir List to write in
  //  
  DGUARD(fDebug, 1, "Define output FMD density calculator");
  TList* d = new TList;
  d->SetOwner();
  d->SetName(GetName());
  dir->Add(d);
  d->Add(fWeightedSum);
  d->Add(fSumOfWeights);
  d->Add(fCorrections);
  d->Add(fAccI);
  d->Add(fAccO);
  d->Add(fMaxWeights);
  d->Add(fLowCuts);

  // TNamed* sigma  = new TNamed("sigma",
  // (fIncludeSigma ? "included" : "excluded"));
  TNamed* maxP   = new TNamed("maxParticle", Form("%d", fMaxParticles));
  TNamed* method = new TNamed("method", 
			      (fUsePoisson ? "Poisson" : "Energy loss"));
  TNamed* phiA   = new TNamed("phiAcceptance", 
			      (fUsePhiAcceptance == 0 ? "disabled" : 
			       fUsePhiAcceptance == 1 ? "particles" :
			       "energy loss"));
  TNamed* etaL   = new TNamed("etaLumping", Form("%d", fEtaLumping));
  TNamed* phiL   = new TNamed("phiLumping", Form("%d", fPhiLumping));
  // TParameter<double>* nxi = new TParameter<double>("nXi", fNXi);

  // d->Add(sigma);
  d->Add(maxP);
  d->Add(method);
  d->Add(phiA);
  d->Add(etaL);
  d->Add(phiL);
  // d->Add(nxi);
  fCuts.Output(d,0);

  TIter    next(&fRingHistos);
  RingHistos* o = 0;
  while ((o = static_cast<RingHistos*>(next()))) {
    o->fPoisson.SetLumping(fEtaLumping, fPhiLumping);
    o->Output(d);
  }
}
//____________________________________________________________________
void
AliFMDDensityCalculator::Print(Option_t* option) const
{
  // 
  // Print information 
  // 
  // Parameters:
  //    option Not used
  //
  char ind[gROOT->GetDirLevel()+3];
  for (Int_t i = 0; i < gROOT->GetDirLevel(); i++) ind[i] = ' ';
  ind[gROOT->GetDirLevel()] = '\0';
  std::cout << ind << ClassName() << ": " << GetName() << '\n'
	    << std::boolalpha 
	    << ind << " Max(particles):         " << fMaxParticles << '\n'
	    << ind << " Poisson method:         " << fUsePoisson << '\n'
	    << ind << " Use phi acceptance:     " << fUsePhiAcceptance << '\n'
	    << ind << " Eta lumping:            " << fEtaLumping << '\n'
	    << ind << " Phi lumping:            " << fPhiLumping << '\n'
	    << std::noboolalpha
	    << std::flush;
  std::cout << ind << " Lower cut:" << std::endl;
  fCuts.Print();
  TString opt(option);
  opt.ToLower();
  if (opt.Contains("nomax")) return;
  
  std::cout << ind << " Max weights:\n";

  for (UShort_t d=1; d<=3; d++) { 
    UShort_t nr = (d == 1 ? 1 : 2);
    for (UShort_t q=0; q<nr; q++) { 
      ind[gROOT->GetDirLevel()]   = ' ';
      ind[gROOT->GetDirLevel()+1] = '\0';
      Char_t      r = (q == 0 ? 'I' : 'O');
      std::cout << ind << " FMD" << d << r << ":";
      ind[gROOT->GetDirLevel()+1] = ' ';
      ind[gROOT->GetDirLevel()+2] = '\0';
      
      const TArrayI& a = (d == 1 ? fFMD1iMax : 
			  (d == 2 ? (r == 'I' ? fFMD2iMax : fFMD2oMax) : 
			   (r == 'I' ? fFMD3iMax : fFMD3oMax)));
      Int_t j = 0;
      for (Int_t i = 0; i < a.fN; i++) { 
	if (a.fArray[i] < 1) continue; 
	if (j % 6 == 0)      std::cout << "\n " << ind;
	j++;
	std::cout << "  " << std::setw(3) << i << ": " << a.fArray[i];
      }
      std::cout << std::endl;
    }
  }
}

//====================================================================
AliFMDDensityCalculator::RingHistos::RingHistos()
  : AliForwardUtil::RingHistos(),
    fEvsN(0), 
    fEvsM(0), 
    fEtaVsN(0),
    fEtaVsM(0),
    fCorr(0),
    fDensity(0),
    fELossVsPoisson(0),
    fPoisson(),
    fELoss(0),
    fELossUsed(0),
    fMultCut(0)
{
  // 
  // Default CTOR
  //
}

//____________________________________________________________________
AliFMDDensityCalculator::RingHistos::RingHistos(UShort_t d, Char_t r)
  : AliForwardUtil::RingHistos(d,r),
    fEvsN(0), 
    fEvsM(0),
    fEtaVsN(0),
    fEtaVsM(0),
    fCorr(0),
    fDensity(0),
    fELossVsPoisson(0),
    fPoisson("ignored"),
    fELoss(0),
    fELossUsed(0),
    fMultCut(0)
{
  // 
  // Constructor
  // 
  // Parameters:
  //    d detector
  //    r ring 
  //
  fEvsN = new TH2D("elossVsNnocorr", 
		   "#Delta E/#Delta E_{mip} vs uncorrected inclusive N_{ch}",
		   250, -.5, 24.5, 250, -.5, 24.5);
  fEvsN->SetXTitle("#Delta E/#Delta E_{mip}");
  fEvsN->SetYTitle("Inclusive N_{ch} (uncorrected)");
  fEvsN->Sumw2();
  fEvsN->SetDirectory(0);

  fEvsM = static_cast<TH2D*>(fEvsN->Clone("elossVsNcorr"));
  fEvsM->SetTitle("#Delta E/#Delta E_{mip} vs corrected inclusive N_{ch}");
  fEvsM->SetDirectory(0);

  fEtaVsN = new TProfile("etaVsNnocorr",
			 "Average inclusive N_{ch} vs #eta (uncorrected)",
			 200, -4, 6);
  fEtaVsN->SetXTitle("#eta");
  fEtaVsN->SetYTitle("#LT N_{ch,incl}#GT (uncorrected)");
  fEtaVsN->SetDirectory(0);
  fEtaVsN->SetLineColor(Color());
  fEtaVsN->SetFillColor(Color());

  fEtaVsM = static_cast<TProfile*>(fEtaVsN->Clone("etaVsNcorr"));
  fEtaVsM->SetTitle("Average inclusive N_{ch} vs #eta (corrected)");
  fEtaVsM->SetYTitle("#LT N_{ch,incl}#GT (corrected)");
  fEtaVsM->SetDirectory(0);


  fCorr = new TProfile("corr", "Average correction", 200, -4, 6);
  fCorr->SetXTitle("#eta");
  fCorr->SetYTitle("#LT correction#GT");
  fCorr->SetDirectory(0);
  fCorr->SetLineColor(Color());
  fCorr->SetFillColor(Color());

  fDensity = new TH2D("inclDensity", "Inclusive N_{ch} density",
		      200, -4, 6, (r == 'I' || r == 'i' ? 20 : 40), 
		      0, 2*TMath::Pi());
  fDensity->SetDirectory(0);
  fDensity->Sumw2();
  fDensity->SetMarkerColor(Color());
  fDensity->SetXTitle("#eta");
  fDensity->SetYTitle("#phi [radians]");
  fDensity->SetZTitle("Inclusive N_{ch} density");

  fELossVsPoisson = new TH2D("elossVsPoisson", 
			     "N_{ch} from energy loss vs from Poission",
			     500, 0, 100, 500, 0, 100);
  fELossVsPoisson->SetDirectory(0);
  fELossVsPoisson->SetXTitle("N_{ch} from #DeltaE");
  fELossVsPoisson->SetYTitle("N_{ch} from Poisson");
  fELossVsPoisson->SetZTitle("Correlation");

  fELoss = new TH1D("eloss", "#Delta/#Delta_{mip} in all strips", 
		    600, 0, 15);
  fELoss->SetXTitle("#Delta/#Delta_{mip} (selected)");
  fELoss->SetYTitle("P(#Delta/#Delta_{mip})");
  fELoss->SetFillColor(Color()-2);
  fELoss->SetFillStyle(3003);
  fELoss->SetLineColor(kBlack);
  fELoss->SetLineStyle(2);
  fELoss->SetLineWidth(2);
  fELoss->SetDirectory(0);

  fELossUsed = static_cast<TH1D*>(fELoss->Clone("elossUsed"));
  fELossUsed->SetTitle("#Delta/#Delta_{mip} in used strips");
  fELossUsed->SetFillStyle(3002);
  fELossUsed->SetLineStyle(1);
  fELossUsed->SetDirectory(0);
  
}
//____________________________________________________________________
AliFMDDensityCalculator::RingHistos::RingHistos(const RingHistos& o)
  : AliForwardUtil::RingHistos(o), 
    fEvsN(o.fEvsN), 
    fEvsM(o.fEvsM),
    fEtaVsN(o.fEtaVsN),
    fEtaVsM(o.fEtaVsM),
    fCorr(o.fCorr),
    fDensity(o.fDensity),
    fELossVsPoisson(o.fELossVsPoisson),
    fPoisson(o.fPoisson),
    fELoss(o.fELoss),
    fELossUsed(o.fELossUsed),
    fMultCut(o.fMultCut)
{
  // 
  // Copy constructor 
  // 
  // Parameters:
  //    o Object to copy from 
  //
}

//____________________________________________________________________
AliFMDDensityCalculator::RingHistos&
AliFMDDensityCalculator::RingHistos::operator=(const RingHistos& o)
{
  // 
  // Assignment operator 
  // 
  // Parameters:
  //    o Object to assign from 
  // 
  // Return:
  //    Reference to this 
  //
  if (&o == this) return *this; 
  AliForwardUtil::RingHistos::operator=(o);
  
  if (fEvsN)           delete fEvsN;
  if (fEvsM)           delete fEvsM;
  if (fEtaVsN)         delete fEtaVsN;
  if (fEtaVsM)         delete fEtaVsM;
  if (fCorr)           delete fCorr;
  if (fDensity)        delete fDensity;
  if (fELossVsPoisson) delete fELossVsPoisson;
  
  fEvsN           = static_cast<TH2D*>(o.fEvsN->Clone());
  fEvsM           = static_cast<TH2D*>(o.fEvsM->Clone());
  fEtaVsN         = static_cast<TProfile*>(o.fEtaVsN->Clone());
  fEtaVsM         = static_cast<TProfile*>(o.fEtaVsM->Clone());
  fCorr           = static_cast<TProfile*>(o.fCorr->Clone());
  fDensity        = static_cast<TH2D*>(o.fDensity->Clone());
  fELossVsPoisson = static_cast<TH2D*>(o.fELossVsPoisson->Clone());
  fPoisson        = o.fPoisson;
  fELoss          = static_cast<TH1D*>(o.fELoss->Clone());
  fELossUsed      = static_cast<TH1D*>(o.fELossUsed->Clone());
  
  return *this;
}
//____________________________________________________________________
AliFMDDensityCalculator::RingHistos::~RingHistos()
{
  // 
  // Destructor 
  //
}


//____________________________________________________________________
void
AliFMDDensityCalculator::RingHistos::Init(const TAxis& /*eAxis*/)
{
  fPoisson.Init(-1,-1);
}

//____________________________________________________________________
void
AliFMDDensityCalculator::RingHistos::Output(TList* dir)
{
  // 
  // Make output 
  // 
  // Parameters:
  //    dir Where to put it 
  //
  TList* d = DefineOutputList(dir);
  d->Add(fEvsN);
  d->Add(fEvsM);
  d->Add(fEtaVsN);
  d->Add(fEtaVsM);
  d->Add(fCorr);
  d->Add(fDensity);
  d->Add(fELossVsPoisson);
  fPoisson.Output(d);
  fPoisson.GetOccupancy()->SetFillColor(Color());
  fPoisson.GetMean()->SetFillColor(Color());
  fPoisson.GetOccupancy()->SetFillColor(Color());
  d->Add(fELoss);
  d->Add(fELossUsed);

  Bool_t inner = (fRing == 'I' || fRing == 'i');
  Int_t nStr = inner ? 512 : 256;
  Int_t nSec = inner ?  20 :  40;
  TAxis x(nStr, -.5, nStr-.5);
  TAxis y(nSec, -.5, nSec-.5);
  x.SetTitle("strip");
  y.SetTitle("sector");
  fPoisson.Define(x, y);

  TParameter<double>* cut = new TParameter<double>("cut", fMultCut);
  d->Add(cut);
}

//____________________________________________________________________
void
AliFMDDensityCalculator::RingHistos::ScaleHistograms(TList* dir, Int_t nEvents)
{
  // 
  // Scale the histograms to the total number of events 
  // 
  // Parameters:
  //    dir     Where the output is 
  //    nEvents Number of events 
  //
  TList* l = GetOutputList(dir);
  if (!l) return; 

  TH1* density = GetOutputHist(l,"inclDensity");
  if (density) density->Scale(1./nEvents);
}

//____________________________________________________________________
//
// EOF
//
	  


