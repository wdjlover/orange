#
# OWWidget.py
# Orange Widget
# A General Orange Widget, from which all the Orange Widgets are derived
#

from OWBaseWidget import *

class OWWidget(OWBaseWidget):
    def __init__( self, parent = None, signalManager = None, title = "Orange Widget", wantGraph = False, wantStatusBar = False, savePosition = True, wantMainArea = 1, noReport = False, showSaveGraph = 1, resizingEnabled = 1, **args):
        """
        Initialization
        Parameters:
            title - The title of the\ widget, including a "&" (for shortcut in about box)
            wantGraph - displays a save graph button or not
        """

        OWBaseWidget.__init__(self, parent, signalManager, title, savePosition = savePosition, resizingEnabled = resizingEnabled, **args)

        self.setLayout(QVBoxLayout())
        self.layout().setMargin(2)

        self.topWidgetPart = OWGUI.widgetBox(self, orientation = "horizontal", margin = 0)
        self.leftWidgetPart = OWGUI.widgetBox(self.topWidgetPart, orientation = "vertical", margin = 0)
        if wantMainArea:
            self.leftWidgetPart.setSizePolicy(QSizePolicy(QSizePolicy.Fixed, QSizePolicy.MinimumExpanding))
            self.leftWidgetPart.updateGeometry()
            self.mainArea = OWGUI.widgetBox(self.topWidgetPart, orientation = "vertical", sizePolicy = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding), margin = 0)
            self.mainArea.layout().setMargin(4)
            self.mainArea.updateGeometry()
        self.controlArea = OWGUI.widgetBox(self.leftWidgetPart, orientation = "vertical", margin = wantMainArea and 0 or 4)

        self.space = self.controlArea

        if wantGraph and showSaveGraph:
            self.buttonBackground = OWGUI.widgetBox(self.leftWidgetPart, orientation = "vertical", margin = 2)
            self.graphButton = OWGUI.button(self.buttonBackground, self, "&Save Graph")
            self.graphButton.setAutoDefault(0)

        self.reportData = None
        if hasattr(self, "sendReport") and not noReport:
            if not hasattr(self, "buttonBackground"):
                self.buttonBackground = OWGUI.widgetBox(self.leftWidgetPart, orientation = "vertical", margin = 2)
            self.reportButton = OWGUI.button(self.buttonBackground, self, "&Report", self.sendReport, debuggingEnabled = 0)

        if wantStatusBar:
            #self.widgetStatusArea = OWGUI.widgetBox(self, orientation = "horizontal", margin = 2)
            self.widgetStatusArea = QFrame(self) 
            self.statusBarIconArea = QFrame(self)
            self.widgetStatusBar = QStatusBar(self) 
            
            self.layout().addWidget(self.widgetStatusArea)
            
            self.widgetStatusArea.setLayout(QHBoxLayout(self.widgetStatusArea))
            self.widgetStatusArea.layout().addWidget(self.statusBarIconArea)
            self.widgetStatusArea.layout().addWidget(self.widgetStatusBar)
            self.widgetStatusArea.layout().setMargin(0)
            self.widgetStatusArea.setFrameShape(QFrame.StyledPanel)
                       
            self.statusBarIconArea.setLayout(QHBoxLayout())
            self.widgetStatusBar.setSizeGripEnabled(0) 
            #self.statusBarIconArea.setFrameStyle (QFrame.Panel + QFrame.Sunken)
            #self.widgetStatusBar.setFrameStyle (QFrame.Panel + QFrame.Sunken)
            #self.widgetStatusBar.setSizePolicy(QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred))
            #self.widgetStatusBar.setSizePolicy(QSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.Preferred))
            #self.widgetStatusBar.updateGeometry()
            #self.statusBarIconArea.setFixedSize(16*2,18)
            self.statusBarIconArea.hide()
            

            # create pixmaps used in statusbar to show info, warning and error messages
            #self._infoWidget, self._infoPixmap = self.createPixmapWidget(self.statusBarIconArea, os.path.join(self.widgetDir + "icons/triangle-blue.png"))
            self._warningWidget = self.createPixmapWidget(self.statusBarIconArea, os.path.join(self.widgetDir + "icons/triangle-orange.png"))
            self._errorWidget = self.createPixmapWidget(self.statusBarIconArea, os.path.join(self.widgetDir + "icons/triangle-red.png"))
        
        

    # status bar handler functions
    def createPixmapWidget(self, parent, iconName):
        w = QLabel(parent)
        parent.layout().addWidget(w)
        w.setFixedSize(16,16)
        w.hide()
        if os.path.exists(iconName):
            w.setPixmap(QPixmap(iconName))
        return w

    def setState(self, stateType, id, text):
        stateChanged = OWBaseWidget.setState(self, stateType, id, text)
        if not stateChanged or not hasattr(self, "widgetStatusArea"):
            return

        iconsShown = 0
        #for state, widget, icon, use in [("Info", self._infoWidget, self._owInfo), ("Warning", self._warningWidget, self._owWarning), ("Error", self._errorWidget, self._owError)]:
        for state, widget, use in [("Warning", self._warningWidget, self._owWarning), ("Error", self._errorWidget, self._owError)]:
            if not widget: continue
            if use and self.widgetState[state] != {}:
                widget.setToolTip("\n".join(self.widgetState[state].values()))
                widget.show()
                iconsShown = 1
            else:
                widget.setToolTip("")
                widget.hide()

        if iconsShown:
            self.statusBarIconArea.show()
        else:
            self.statusBarIconArea.hide()

        #if (stateType == "Info" and self._owInfo) or (stateType == "Warning" and self._owWarning) or (stateType == "Error" and self._owError):
        if (stateType == "Warning" and self._owWarning) or (stateType == "Error" and self._owError):
            if text:
                self.setStatusBarText(stateType + ": " + text)
            else:
                self.setStatusBarText("")
        self.updateStatusBarState()
        #qApp.processEvents()

    def updateStatusBarState(self):
        if not hasattr(self, "widgetStatusArea"):
            return
        if self._owShowStatus and (self.widgetState["Warning"] != {} or self.widgetState["Error"] != {}):
            self.widgetStatusArea.show()
        else:
            self.widgetStatusArea.hide()

    def setStatusBarText(self, text, timeout = 5000):
        if hasattr(self, "widgetStatusBar"):
            self.widgetStatusBar.showMessage(" " + text, timeout)

    def startReport(self, name, needDirectory = False):
        if self.reportData:
            print "Cannot open a new report when an old report is still active"
            return False
        self.reportName = name
        self.reportData = ""
        if needDirectory:
            import OWReport
            return OWReport.reportFeeder.createDirectory()
        else:
            return True

    def reportSection(self, title):
        self.reportData += "<H2>%s</H2>\n" % title

    def reportSubsection(self, title):
        self.reportData += "<H3>%s</H3>\n" % title

    def reportList(self, items):
        self.startReportList()
        for item in items:
            self.addToReportList(item)
        self.finishReportList()

    def reportImage(self, filename):
        self.reportData += '<IMG src="%s"/>' % filename

    def startReportList(self):
        self.reportData += "<UL>\n"

    def addToReportList(self, item):
        self.reportData += "    <LI>%s</LI>\n" % item

    def finishReportList(self):
        self.reportData += "</UL>\n"

    def reportSettings(self, settingsList, closeList = True):
        self.startReportList()
        for item in settingsList:
            if item:
                self.addToReportList("<B>%s:</B> %s" % item)
        if closeList:
            self.finishReportList()

    def reportRaw(self, text):
        self.reportData += text

    def finishReport(self):
        import OWReport
        OWReport.reportFeeder(self.reportName, self.reportData or "")
        self.reportData = None

if __name__ == "__main__":
    a=QApplication(sys.argv)
    ow=OWWidget()
    ow.show()
    a.exec_()
