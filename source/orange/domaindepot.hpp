#ifndef __DOMAINDEPOT_HPP
#define __DOMAINDEPOT_HPP

#include <vector>
#include <list>
using namespace std;

#include "getarg.hpp"
#include "orvector.hpp"
#include "vars.hpp"
#include "domain.hpp"

class TMetaVector;

#define TDomainList TOrangeVector<PDomain> 
VWRAPPER(DomainList)

// For each attribute, the correspongin element of multimapping gives
//   domains and position in domains in which the attribute appears
typedef vector<vector<pair<int, int> > > TDomainMultiMapping;


class ORANGE_API TDomainDepot
{
/* XXX Domain depot has no wrapped pointers to anything, thus it doesn't own any references.
       If you add any, you should implement DomainDepot_traverse and DomainDepot_clear in 
       Python interface. */
public:
  class TAttributeDescription {
  public:
    PVariable preparedVar;
    
    string name;
    int varType;
    string typeDeclaration;
    bool ordered;
    TStringList fixedOrderValues; // these have a fixed order
    map<string, int> values; // frequencies of values
    TMultiStringParameters userFlags;
    
    TAttributeDescription(const string &, const int &, const string &, bool = false);
    TAttributeDescription(const string &, const int &);
    TAttributeDescription(PVariable);
    
    void addValue(const string &s);
  };

  ~TDomainDepot();

  typedef vector<TAttributeDescription> TAttributeDescriptions;
  typedef vector<TAttributeDescription *> TPAttributeDescriptions;
  static void pattrFromtAttr(TDomainDepot::TAttributeDescriptions &, TDomainDepot::TPAttributeDescriptions &);

  static bool checkDomain(const TDomain *, const TPAttributeDescriptions *attributes, bool hasClass,
                          const TPAttributeDescriptions *metas, int *metaIDs = NULL);

  PDomain prepareDomain(TPAttributeDescriptions *attributes, bool hasClass,
                        TPAttributeDescriptions *classDescriptions,
                        TPAttributeDescriptions *metas, const int createNewOn,
                        vector<int> &status, vector<pair<int, int> > &metaStatus);

  static void destroyNotifier(TDomain *domain, void *);

  static PVariable createVariable_Python(const string &typeDeclaration, const string &name);
  static PVariable makeVariable(TAttributeDescription &desc, int &status, const int &createNewOn = TVariable::Incompatible);


private:
  list<TDomain *> knownDomains;
};



PDomain combineDomains(PDomainList sources, TDomainMultiMapping &mapping);
void computeMapping(PDomain destination, PDomainList sources, TDomainMultiMapping &mapping);


// global domain depot
extern TDomainDepot domainDepot;

#endif
