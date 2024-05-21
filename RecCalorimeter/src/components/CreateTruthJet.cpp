#include "CreateTruthJet.h"

// std
#include <vector>
#include <math.h>



DECLARE_COMPONENT(CreateTruthJet)

CreateTruthJet::CreateTruthJet(const std::string& name, ISvcLocator* svcLoc) : Transformer(name, svcLoc,
                    KeyValue("InputCollection", "MCParticles"),
                    KeyValue("OutputCollection", "TruthJets")) {
  declareProperty("JetAlg", m_jetAlg, "Name of jet clustering algorithm");
  declareProperty("JetRadius", m_jetRadius, "Jet clustering radius");
  declareProperty("MinPt", m_minPt, "Minimum pT for saved jets");
}



StatusCode CreateTruthJet::initialize() {
  if (m_jetAlgMap.find(m_jetAlg) == m_jetAlgMap.end()) {
    error() << m_jetAlg << " is not in the list of supported jet algorithms" << endmsg;
    return StatusCode::FAILURE;
  }

  return StatusCode::SUCCESS;
}

colltype_out  CreateTruthJet::operator()(const colltype_in& input) const{
  edm4hep::ReconstructedParticleCollection edmJets = edm4hep::ReconstructedParticleCollection();


  fastjet::JetDefinition* jetDef = new fastjet::JetDefinition( m_jetAlgMap.at(m_jetAlg), m_jetRadius);


  std::vector<fastjet::PseudoJet> clustersPJ;
  int i=0;
  for(auto particle: input){
    fastjet::PseudoJet clusterPJ(particle.getMomentum().x, particle.getMomentum().y, particle.getMomentum().z, particle.getEnergy());
    clusterPJ.set_user_info(new ClusterInfo(i));
    clustersPJ.push_back(clusterPJ);
    i++;
  }


  fastjet::ClusterSequence clustSeq(clustersPJ, *jetDef);
  std::vector <fastjet::PseudoJet> inclusiveJets = fastjet::sorted_by_pt(clustSeq.inclusive_jets(m_minPt));
  for(auto cjet : inclusiveJets){
    edm4hep::MutableReconstructedParticle jet;
    jet.setMomentum(edm4hep::Vector3f(cjet.px(), cjet.py(), cjet.pz()));
    jet.setEnergy(cjet.e());
    jet.setMass(cjet.m());

    std::vector<fastjet::PseudoJet> constits = cjet.constituents();
    for(auto constit : constits){
      edm4hep::MutableReconstructedParticle jetInput;
      jetInput.setMomentum(edm4hep::Vector3f(constit.px(), constit.py(), constit.pz()));
      jetInput.setEnergy(constit.e());
      jetInput.setMass(constit.m());

      int index = constit.user_info<ClusterInfo>().index();
      jetInput.setPDG((input)[index].getPDG());

      jet.addToParticles(jetInput);
    }

    edmJets.push_back(jet);
  }

  return edmJets;
}



