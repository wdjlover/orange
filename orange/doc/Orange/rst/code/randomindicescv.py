# Description: Constructs indices for cross-validation
# Category:    sampling
# Classes:     MakeRandomIndices, MakeRandomIndicesCV
# Uses:        lenses
# Referenced:  RandomIndices.htm

import Orange
data = Orange.data.Table("lenses")
print "Indices for ordinary 10-fold CV"
print Orange.data.sample.SubsetIndicesCV(data)
print "Indices for 5 folds on 10 examples"
print Orange.data.sample.SubsetIndicesCV(10, folds=5)