/*
    This file is part of Orange.

    Orange is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Orange is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Orange; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Authors: Janez Demsar, Blaz Zupan, 1996--2002
    Contact: janez.demsar@fri.uni-lj.si
*/


#include "random.hpp"

#include "vars.hpp"
#include "domain.hpp"
#include "examples.hpp"
#include "examplegen.hpp"
#include "table.hpp"
#include "meta.hpp"


#include "filter.hpp"
#include "trindex.hpp"
#include "spec_gen.hpp"
#include "stladdon.hpp"
#include "tabdelim.hpp"
#include "discretize.hpp"
#include "classfromvar.hpp"
#include "cost.hpp"

#include <string>
#include "preprocessors.ppp"

DEFINE_TOrangeMap_classDescription(TOrangeMap_KV, PVariable, PValueFilter, "VariableFilterMap")
DEFINE_TOrangeMap_classDescription(TOrangeMap_K, PVariable, float, "VariableFloatMap")

#ifdef _MSC_VER
  #pragma warning (disable : 4100) // unreferenced local parameter (macros name all arguments)
#endif


PExampleGenerator TPreprocessor::filterExamples(PFilter filter, PExampleGenerator generator)
{ TFilteredGenerator fg(filter, generator);
  return PExampleGenerator(mlnew TExampleTable(PExampleGenerator(fg))); 
}



TPreprocessor_ignore::TPreprocessor_ignore()
: attributes(mlnew TVarList())
{}


TPreprocessor_ignore::TPreprocessor_ignore(PVarList attrs)
: attributes(attrs)
{}


PExampleGenerator TPreprocessor_ignore::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{
  PDomain outDomain = CLONE(TDomain, gen->domain);
  PITERATE(TVarList, vi, attributes)
    if (!outDomain->delVariable(*vi))
      if (*vi == outDomain->classVar)
        outDomain->removeClass();
      else
        raiseError("attribute '%s' not found", (*vi)->name.c_str());

  newWeight = weightID;
  return PExampleGenerator(mlnew TExampleTable(outDomain, gen));
}



TPreprocessor_select::TPreprocessor_select()
: attributes(mlnew TVarList())
{}


TPreprocessor_select::TPreprocessor_select(PVarList attrs)
: attributes(attrs)
{}


PExampleGenerator TPreprocessor_select::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{
  PDomain outDomain = CLONE(TDomain, gen->domain);
  TVarList::const_iterator bi(attributes->begin()), be(attributes->end());

  PITERATE(TVarList, vi, gen->domain->attributes)
    if (find(bi, be, *vi)==be)
      outDomain->delVariable(*vi);

  if (find(bi, be, outDomain->classVar) == be)
    outDomain->removeClass();

  newWeight = weightID;
  return PExampleGenerator(mlnew TExampleTable(outDomain, gen));
}




TPreprocessor_drop::TPreprocessor_drop()
: values(mlnew TVariableFilterMap())
{}


TPreprocessor_drop::TPreprocessor_drop(PVariableFilterMap avalues)
: values(avalues)
{}


PExampleGenerator TPreprocessor_drop::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ TValueFilterList *dropvalues = mlnew TValueFilterList();
  PValueFilterList wdropvalues = dropvalues;
  const TDomain &domain = gen->domain.getReference();
  PITERATE(TVariableFilterMap, vi, values) {
    TValueFilter *vf = CLONE(TValueFilter, (*vi).second);
    dropvalues->push_back(vf); // this wraps it!
    vf->position = domain.getVarNum((*vi).first);
  }

  newWeight = weightID;
  return filterExamples(mlnew TFilter_values(wdropvalues, true, true, gen->domain), gen);
}

  

TPreprocessor_take::TPreprocessor_take()
: values(mlnew TVariableFilterMap())
{}


TPreprocessor_take::TPreprocessor_take(PVariableFilterMap avalues)
: values(avalues)
{}


PExampleGenerator TPreprocessor_take::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ 
  newWeight = weightID;
  return filterExamples(constructFilter(values, gen->domain), gen);
}


PFilter TPreprocessor_take::constructFilter(PVariableFilterMap values, PDomain domain)
{ TValueFilterList *dropvalues = mlnew TValueFilterList();
  PValueFilterList wdropvalues = dropvalues;
  PITERATE(TVariableFilterMap, vi, values) {
    TValueFilter *vf = CLONE(TValueFilter, (*vi).second);
    dropvalues->push_back(vf); // this wraps it!
    vf->position = domain->getVarNum((*vi).first);
  }
  return mlnew TFilter_values(wdropvalues, true, false, domain);
}



PExampleGenerator TPreprocessor_removeDuplicates::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ PExampleGenerator table = mlnew TExampleTable(gen);

  if (weightID)
    newWeight = weightID;
  else {
    newWeight = getMetaID();
    table.AS(TExampleTable)->addMetaAttribute(newWeight, TValue(float(1.0)));
  }

  table.AS(TExampleTable)->removeDuplicates(newWeight);
  return table;
}



PExampleGenerator TPreprocessor_dropMissing::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ newWeight = weightID;
  return filterExamples(mlnew TFilter_hasSpecial(true), gen);
}


PExampleGenerator TPreprocessor_takeMissing::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ newWeight = weightID;
  return filterExamples(mlnew TFilter_hasSpecial(false), gen);
}



PExampleGenerator TPreprocessor_dropMissingClasses::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ newWeight = weightID;
  return filterExamples(mlnew TFilter_hasClassValue(false), gen);
}



PExampleGenerator TPreprocessor_takeMissingClasses::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ newWeight = weightID;
  return filterExamples(mlnew TFilter_hasClassValue(true), gen);
}



TPreprocessor_addNoise::TPreprocessor_addNoise()
: proportions(mlnew TVariableFloatMap()),
  defaultProportion(0.0)  
{}


TPreprocessor_addNoise::TPreprocessor_addNoise(PVariableFloatMap probs, const float &defprob)
: proportions(probs),
  defaultProportion(defprob)  
{}


PExampleGenerator TPreprocessor_addNoise::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ 
  newWeight = weightID;

  if (!proportions && (defaultProportion<=0.0))
    return mlnew TExampleTable(gen);

  const TDomain &domain = gen->domain.getReference();
  vector<pair<int, float> > ps;
  vector<bool> attributeUsed(domain.attributes->size(), false);

  TExampleTable *table = mlnew TExampleTable(gen);
  PExampleGenerator wtable = table;

  const int n = table->size();
  TMakeRandomIndices2 makerind;

  if (proportions)
    PITERATE(TVariableFloatMap, vi, proportions) {
      const TVariable &var = (*vi).first.getReference();
      const int idx = domain.getVarNum((*vi).first);
      if ((*vi).second > 0.0) {
        PLongList rind = makerind(n, 1 - (*vi).second);
        int eind = 0;
        PITERATE(TLongList, ri, rind) {
          if (*ri)
            (*table)[eind][idx] = var.randomValue();
          eind++;
        }
      }
      if ((idx >= 0) && (idx < attributeUsed.size())) // not a class
        attributeUsed[idx] = true;
    }


  if (defaultProportion > 0.0) {
    TVarList::const_iterator vi(table->domain->attributes->begin());
    int idx = 0;
    const vector<bool>::const_iterator bb(attributeUsed.begin()), be(attributeUsed.end());
    for(vector<bool>::const_iterator bi(bb); bi != be; bi++, vi++, idx++)
      if (!*bi) {
        const TVariable &var = (*vi).getReference();
        PLongList rind = makerind(n, 1 - defaultProportion);

        int eind = 0;
        PITERATE(TLongList, ri, rind) {
          if (*ri)
            (*table)[eind][idx] = var.randomValue();
          eind++;
        }
      }
  }

  return wtable;
} 



TPreprocessor_addGaussianNoise::TPreprocessor_addGaussianNoise()
: deviations(mlnew TVariableFloatMap()),
  defaultDeviation(0.0)
{}



TPreprocessor_addGaussianNoise::TPreprocessor_addGaussianNoise(PVariableFloatMap devs, const float &defdev)
: deviations(devs),
  defaultDeviation(defdev)  
{}


/* For Gaussian noise we use TGaussianNoiseGenerator; the advantage against going
   attribute by attribute (like in addNoise) is that it might require less paging
   on huge datasets. */
PExampleGenerator TPreprocessor_addGaussianNoise::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ 
  newWeight = weightID;

  if (!deviations && (defaultDeviation<=0.0))
    return mlnew TExampleTable(gen);

  const TDomain &domain = gen->domain.getReference();
  vector<pair<int, float> > ps;
  vector<bool> attributeUsed(domain.attributes->size(), false);
  
  if (deviations)
    PITERATE(TVariableFloatMap, vi, deviations) {
      PVariable var = (*vi).first;
      if (var->varType != TValue::FLOATVAR)
        raiseError("attribute '%s' is not continuous", var->name.c_str());

      const int pos = domain.getVarNum(var);
      ps.push_back(pair<int, float>(pos, (*vi).second));

      if ((pos >= 0) && (pos < attributeUsed.size()))
        attributeUsed[pos] = true;
    }
  
  if (defaultDeviation) {
    TVarList::const_iterator vi(domain.attributes->begin());
    const vector<bool>::const_iterator bb = attributeUsed.begin();
    const_ITERATE(vector<bool>, bi, attributeUsed) {
      if (!*bi && ((*vi)->varType == TValue::FLOATVAR))
        ps.push_back(pair<int, float>(bi-bb, defaultDeviation));
      vi++;
    }
  }

  TGaussianNoiseGenerator gg = TGaussianNoiseGenerator(ps, gen);
  return PExampleGenerator(mlnew TExampleTable(PExampleGenerator(gg)));
}



TPreprocessor_addMissing::TPreprocessor_addMissing()
: proportions(mlnew TVariableFloatMap()),
  defaultProportion(0.0),
  specialType(valueDK)
{}


TPreprocessor_addMissing::TPreprocessor_addMissing(PVariableFloatMap probs, const float &defprob, const int &st)
: proportions(probs),
  defaultProportion(defprob),
  specialType(st)
{}


PExampleGenerator TPreprocessor_addMissing::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{
  newWeight = weightID;

  if (!proportions && (defaultProportion<=0.0))
    return mlnew TExampleTable(gen);

  const TDomain &domain = gen->domain.getReference();
  vector<pair<int, float> > ps;
  vector<bool> attributeUsed(domain.attributes->size(), false);

  TExampleTable *table = mlnew TExampleTable(gen);
  PExampleGenerator wtable = table;

  const int n = table->size();
  TMakeRandomIndices2 makerind;

  if (proportions)
    PITERATE(TVariableFloatMap, vi, proportions) {
      const int idx = domain.getVarNum((*vi).first);
      PLongList rind = makerind(n, 1 - (*vi).second);
      if ((*vi).second > 0.0) {
        const unsigned char &varType = (*vi).first->varType;
        int eind = 0;
        PITERATE(TLongList, ri, rind) {
          if (*ri)
            (*table)[eind][idx] = TValue(varType, specialType);
          eind++;
        }
      }
      if ((idx >= 0) && (idx < attributeUsed.size())) // not a class
        attributeUsed[idx] = true;
    }


  if (defaultProportion > 0.0) {
    TVarList::const_iterator vi(table->domain->attributes->begin());
    int idx = 0;
    const vector<bool>::const_iterator bb(attributeUsed.begin()), be(attributeUsed.end());
    for(vector<bool>::const_iterator bi(bb); bi != be; bi++, vi++, idx++)
      if (!*bi) {
        PLongList rind = makerind(n, 1 - defaultProportion);
        const unsigned char &varType = (*vi)->varType;

        int eind = 0;
        PITERATE(TLongList, ri, rind) {
          if (*ri)
            (*table)[eind][idx] = TValue(varType, specialType);
          eind++;
        }
      }
  }

  return wtable;
}



TPreprocessor_addClassNoise::TPreprocessor_addClassNoise(const float &cn)
: proportion(cn)
{}


PExampleGenerator TPreprocessor_addClassNoise::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{
  if (!gen->domain->classVar)
    raiseError("Class-less domain");
  if (gen->domain->classVar->varType != TValue::INTVAR)
    raiseError("Discrete class value expected");

  TExampleTable *table = mlnew TExampleTable(gen);
  PExampleGenerator wtable = table;

  if (proportion>0.0) {
    TMakeRandomIndices2 mri2;
    PLongList rind(mri2(table->size(), 1-proportion));

    const TVariable &classVar = table->domain->classVar.getReference();
    int eind = 0;
    PITERATE(TLongList, ri, rind) {
      if (*ri)
        (*table)[eind].setClass(classVar.randomValue());
      eind++;
    }
  }

  newWeight = weightID;
  return wtable;
}



TPreprocessor_addGaussianClassNoise::TPreprocessor_addGaussianClassNoise(const float &dev)
: deviation(dev)
{}


PExampleGenerator TPreprocessor_addGaussianClassNoise::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{
  PVariable classVar = gen->domain->classVar;

  if (!classVar)
    raiseError("Class-less domain");
  if (classVar->varType != TValue::FLOATVAR)
    raiseError("Class '%s' is not continuous", gen->domain->classVar->name.c_str());

  newWeight = weightID;

  if (deviation>0.0) {
    vector<pair<int, float> > deviations;
    deviations.push_back(pair<int, float>(gen->domain->attributes->size(), deviation));
    TGaussianNoiseGenerator gngen(deviations, gen);
    return PExampleGenerator(mlnew TExampleTable(PExampleGenerator(gngen)));
  }

  else
    return mlnew TExampleTable(gen);
}


TPreprocessor_addMissingClasses::TPreprocessor_addMissingClasses(const float &cm, const int &st)
: proportion(cm),
  specialType(st)
{}
  
  
PExampleGenerator TPreprocessor_addMissingClasses::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{
  if (!gen->domain->classVar)
    raiseError("Class-less domain");

  TExampleTable *table = mlnew TExampleTable(gen);
  PExampleGenerator wtable = table;

  if (proportion>0.0) {
    TMakeRandomIndices2 mri2;
    PLongList rind(mri2(table->size(), 1-proportion));

    const TVariable &classVar = table->domain->classVar.getReference();
    const int &varType = classVar.varType;
    int eind = 0;
    PITERATE(TLongList, ri, rind) {
      if (*ri)
        (*table)[eind].setClass(TValue(varType, specialType));
      eind++;
    }
  }

  newWeight = weightID;
  return wtable;
}



TPreprocessor_addClassWeight::TPreprocessor_addClassWeight()
: classWeights(mlnew TFloatList),
  equalize(false)
{}


TPreprocessor_addClassWeight::TPreprocessor_addClassWeight(PFloatList cw, const bool &eq)
: equalize(eq),
  classWeights(cw)
{}


PExampleGenerator TPreprocessor_addClassWeight::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{
  if (!gen->domain->classVar || (gen->domain->classVar->varType != TValue::INTVAR))
    raiseError("Class-less domain or non-discrete class");

  TExampleTable *table = mlnew TExampleTable(gen);
  PExampleGenerator wtable = table;

  const int nocl = gen->domain->classVar->noOfValues();

  if (!equalize && !classWeights->size() || !nocl) {
    newWeight = 0;
    return wtable;
  }

  if (classWeights && classWeights->size() && (classWeights->size() != nocl))
    raiseError("size of classWeights should equal the number of classes");


  vector<float> weights;

  if (equalize) {
    PDistribution dist(getClassDistribution(gen, weightID));
    const TDiscDistribution &ddist = CAST_TO_DISCDISTRIBUTION(dist);
    if (ddist.size() > nocl)
      raiseError("there are out-of-range classes in the data (attribute descriptor has too few values)");

    if (classWeights && classWeights->size()) {
      float total = 0.0;
      float tot_w = 0.0;
      vector<float>::const_iterator cwi(classWeights->begin());
      TDiscDistribution::const_iterator di(ddist.begin()), de(ddist.end());
      for(; di!=de; di++, cwi++) {
        total += *di * *cwi;
        tot_w += *cwi;
      }
      if (total == 0.0) {
        newWeight = 0;
        return wtable;
      }

      float fact = tot_w * (ddist.abs / total);
      PITERATE(vector<float>, wi, classWeights)
        weights.push_back(*wi * fact);
    }

    else { // no class weights, only equalization
      int noNullClasses = 0;
      { const_ITERATE(TDiscDistribution, di, ddist)
          if (*di>0.0)
            noNullClasses++;
      }
      const float N = ddist.abs;
      const_ITERATE(TDiscDistribution, di, ddist)
        if (*di>0.0)
          weights.push_back(N / noNullClasses / *di);
        else
          weights.push_back(1.0);
    }
  }

  else  // no equalization, only weights
    weights = classWeights.getReference();

  newWeight = getMetaID();
  PEITERATE(ei, table)
    (*ei).setMeta(newWeight, TValue(WEIGHT(*ei) * weights[(*ei).getClass().intV]));

  return wtable;
}



PDistribution kaplanMeier(PExampleGenerator gen, const int &outcomeIndex, TValue &failValue, const int &timeIndex, const int &weightID);
PDistribution bayesSurvival(PExampleGenerator gen, const int &outcomeIndex, TValue &failValue, const int &timeIndex, const int &weightID, const float &maxTime);

TPreprocessor_addCensorWeight::TPreprocessor_addCensorWeight()
: outcomeVar(),
  timeVar(),
  eventValue(),
  method(km),
  maxTime(0.0),
  addComplementary(false)
{}


TPreprocessor_addCensorWeight::TPreprocessor_addCensorWeight(PVariable ov, PVariable tv, const TValue &ev, const int &me, const float &mt)
: outcomeVar(ov),
  timeVar(tv),
  eventValue(ev),
  method(me),
  maxTime(0.0),
  addComplementary(false)
{}

void TPreprocessor_addCensorWeight::addExample(TExampleTable *table, const int &weightID, const TExample &example, const float &weight, const int &complementary, const float &compWeight)
{ 
  TExample ex = example;

  ex.setMeta(weightID, TValue(weight));
  table->addExample(ex);

  if ((complementary >= 0) && (compWeight>0.0)) {
    ex.setClass(TValue(complementary));
    ex.setMeta(weightID, TValue(compWeight));
    table->addExample(ex);
  }
}


PExampleGenerator TPreprocessor_addCensorWeight::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ 
  if (eventValue.isSpecial())
    raiseError("'eventValue' not set");

  if (eventValue.varType != TValue::INTVAR)
    raiseError("'eventValue' invalid (discrete value expected)");

  const int failIndex = eventValue.intV;

  int outcomeIndex;
  if (outcomeVar) {
    outcomeIndex = gen->domain->getVarNum(outcomeVar, false);
    if (outcomeIndex==ILLEGAL_INT)
      raiseError("outcomeVar not found in domain");
  }
  else
    if (gen->domain->classVar)
      outcomeIndex = gen->domain->attributes->size();
    else
      raiseError("'outcomeVar' not set and the domain is class-less");

  int complementary = addComplementary ? eventValue.intV : -1;

  checkProperty(timeVar);
  int timeIndex = gen->domain->getVarNum(timeVar, false);
  if (timeIndex==ILLEGAL_INT)
    raiseError("'timeVar' not found in domain");

  TExampleTable *table = mlnew TExampleTable(gen->domain);
  PExampleGenerator wtable = table;

  if (method == linear) {
    float thisMaxTime = maxTime;
    if (thisMaxTime<=0.0)
      PEITERATE(ei, table) {
        const TValue &tme = (*ei)[timeIndex];
        if (!tme.isSpecial()) {
          if (tme.varType != TValue::FLOATVAR)
            raiseError("invalid time (continuous attribute expected)");
          else
            if (tme.floatV>thisMaxTime)
              thisMaxTime = tme.floatV;
        }
      }

    if (thisMaxTime<=0.0)
      raiseError("invalid time values (max<=0)");

    newWeight = getMetaID();
    PEITERATE(ei, gen) {
      if (!(*ei)[outcomeIndex].isSpecial() && (*ei)[outcomeIndex].intV==failIndex)
        addExample(table, newWeight, *ei, WEIGHT(*ei), complementary);
      else {
        const TValue &tme = (*ei)[timeIndex];
        // need to check it again -- the above check is only run if maxTime is not given
        if (tme.varType != TValue::FLOATVAR)
          raiseError("invalid time (continuous attribute expected)");

        if (!tme.isSpecial())
          addExample(table, newWeight, *ei, WEIGHT(*ei) * (tme.floatV>thisMaxTime ? 1.0 : tme.floatV / thisMaxTime), complementary);
      }
    }
  }

  else if ((method == km) || (method == bayes)) {
    if ((km==bayes) && (maxTime<=0.0))
      raiseError("'maxTime' should be set when 'method' is 'Bayes'");
      
    PDistribution KM = (method == km) ? kaplanMeier(gen, outcomeIndex, eventValue, timeIndex, weightID)
                                      : bayesSurvival(gen, outcomeIndex, eventValue, timeIndex, weightID, maxTime);

    float KM_max = maxTime>0.0 ? KM->p(maxTime) : (*KM.AS(TContDistribution)->distribution.rbegin()).second;

    newWeight = getMetaID();
    PEITERATE(ei, gen) {
      if (!(*ei)[outcomeIndex].isSpecial() && (*ei)[outcomeIndex].intV==failIndex)
        addExample(table, newWeight, *ei, WEIGHT(*ei), -1);
      else {
        const TValue &tme = (*ei)[timeIndex];
        if (tme.varType != TValue::FLOATVAR)
          raiseError("invalid time (continuous attribute expected)");
        if (tme.varType != TValue::FLOATVAR)
          raiseError("invalid time (continuous value expected)");
        if (!tme.isSpecial()) {
          if (tme.floatV > maxTime)
            addExample(table, newWeight, *ei, WEIGHT(*ei), -1);
          else {
            float KM_t = KM->p(tme.floatV);
            if (method==km) {
              if (KM_t>0) {
                float origw = WEIGHT(*ei);
                float fact = KM_max/KM_t;
                addExample(table, newWeight, *ei, origw*fact, complementary, origw*(1-fact));
              }
            }
            else {
              float origw = WEIGHT(*ei);
              addExample(table, newWeight, *ei, origw*KM_t, complementary, origw*(1-KM_t));
            }
          }
        }
      }
    }
  }

  else
    raiseError("unknown weighting method");

  return wtable;
}
  


TPreprocessor_discretize::TPreprocessor_discretize()
: attributes(),
  discretizeClass(false),
  method()
{}


TPreprocessor_discretize::TPreprocessor_discretize(PVarList attrs, const bool &nocl, PDiscretization meth)
: attributes(attrs),
  discretizeClass(nocl),
  method(meth)
{}


PExampleGenerator TPreprocessor_discretize::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ 
  checkProperty(method);

  const TDomain &domain = gen->domain.getReference();

  vector<int> discretizeId;
  if (attributes && attributes->size()) {
    PITERATE(TVarList, vi, attributes)
      discretizeId.push_back(domain.getVarNum(*vi));
  }
  else {
    int idx = 0;
    const_PITERATE(TVarList, vi, domain.attributes) {
      if ((*vi)->varType==TValue::FLOATVAR)
        discretizeId.push_back(idx);
      idx++;
    }
    if (discretizeClass && (domain.classVar->varType == TValue::FLOATVAR))
      discretizeId.push_back(idx);
  }

  newWeight = weightID;
  return mlnew TExampleTable(PDomain(mlnew TDiscretizedDomain(gen, discretizeId, weightID, method)), gen);
}




TPreprocessor_filter::TPreprocessor_filter(PFilter filt)
: filter(filt)
{}

PExampleGenerator TPreprocessor_filter::operator()(PExampleGenerator gen, const int &weightID, int &newWeight)
{ checkProperty(filter);
  newWeight = weightID;
  return filterExamples(filter, gen);
}
