
var PlayerFactory = Components.Constructor("@c-base.org/avgplayer;1","IAVGPlayer");
var AVGPlayer = new PlayerFactory();

function JSEvalKruecke(){} // constructor
JSEvalKruecke.prototype = {

    callEval: function (code) 
    { 
        return eval(code);
    },

    QueryInterface: function (iid) {
        if (!iid.equals(Components.interfaces.IJSEvalKruecke))
        {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }
        return this;
    }
}

function quitTimeout()
{
    print ("JS: timeout()");
    AVGPlayer.stop();
}

function interval()
{
    dump (".");
}

function onMouseMove()
{
//    print ("JS: onMouseMove()");
}

function onMouseOver()
{
    print ("JS: onMouseOver()");
}
function onMouseOut()
{
    print ("JS: onMouseOut()");
}

function mainMouseUp()
{
    print ("JS: mainMouseUp()");
}

function mainMouseDown()
{
    print ("JS: mainMouseDown()");
}

function onMouseUp()
{
    var Event = AVGPlayer.getCurEvent();
    print ("JS: Event (type= "+Event.getType()+", pos=("+Event.getXPos()+
           ", "+Event.getYPos()+"), state="+Event.getMouseButtonState()+")");
    var Node = Event.getElement();
    print ("    Node: "+Node);
}

var timerid;

function tryLoadFile(fileName)
{
    var ok = AVGPlayer.loadFile("../tests/"+fileName, new JSEvalKruecke());
    if (!ok) {
        print ("js: AVGPlayer.loadFile returned false");
    }
    return ok
}

function playAVG (fileName) 
{

    print ("---- Playing "+fileName+"----");
    AVGPlayer.setEventDebugLevel(2);
    var ok = tryLoadFile(fileName);
    if (ok) {
        timerid = AVGPlayer.setInterval(40, "interval();");
        AVGPlayer.setTimeout(5000, "quitTimeout();");
        AVGPlayer.setTimeout(500, "AVGPlayer.clearInterval(timerid);");
        AVGPlayer.setTimeout(1000, "AVGPlayer.setEventDebugLevel(1);");
        AVGPlayer.setTimeout(2000, "AVGPlayer.setEventDebugLevel(0);");
        AVGPlayer.play();
    }
}

function dumpNodes()
{
    print ("---- dumpNodes: testing node accessors ----");
    var ok = tryLoadFile("avg.avg");
    if (ok) {
        var rootNode = AVGPlayer.getRootNode();
        var numChildren = rootNode.getNumChildren();
        print("  Root node id: "+rootNode.getID()+" ("+numChildren+" children.");
        var i;
        for (i=0; i<numChildren; i++) {
            var curChild = rootNode.getChild(i);
            var parent = curChild.getParent();
            var l = curChild.getIntAttr("Left");
            var t = curChild.getIntAttr("Top");
            var r = curChild.getIntAttr("Right");
            var b = curChild.getIntAttr("Bottom");
            print("    Child node id: "+curChild.getID()+" (Parent: "+parent.getID()
                    +", Pos: ("+l+","+t+","+r+","+b+"), type: "+curChild.getType()+")");
        }
        AVGPlayer.setTimeout(10, "quitTimeout()");
        AVGPlayer.play();
    }
}

function moveit() {
    var node = AVGPlayer.getElementByID("nestedimg1");
    var l = node.getIntAttr("Left");
    node.setIntAttr("Left", l+1);
    var r = node.getIntAttr("Right");
    node.setIntAttr("Right", r+1);
    var Opacity = node.getFloatAttr("Opacity");
    node.setFloatAttr("Opacity", Opacity-0.01);
/*    var z = node.getIntAttr("Z");
    if (z == 1) {
        z = 3;
    } else {
        z = 1;
    }
    node.setIntAttr("Z", z);*/
}

function testAnimation()
{
    print ("---- testing node accessors ----");
    var ok = tryLoadFile("avg.avg");
    if (ok) {
        var node = AVGPlayer.getElementByID("nestedimg1");
        print("    Node id: "+node.getID());
        AVGPlayer.setInterval(10,"moveit();");
        AVGPlayer.setTimeout(5000,"quitTimeout();");
        AVGPlayer.play();
    }
}

function setExcl(i)
{
    var node = AVGPlayer.getElementByID("switch");
    node.setIntAttr("ActiveChild", i);
}

function testExcl()
{
    print ("---- testing node accessors ----");
    var ok = tryLoadFile("excl.avg");
    if (ok) {
        AVGPlayer.setTimeout(300,"setExcl(0);");
        AVGPlayer.setTimeout(600,"setExcl(1);");
        AVGPlayer.setTimeout(900,"setExcl(2);");
        AVGPlayer.setTimeout(1200,"setExcl(3);");
        AVGPlayer.setTimeout(1500,"quitTimeout();");
        AVGPlayer.play();
    }
}

dumpNodes();
testAnimation();
testExcl();

playAVG("empty.avg");
playAVG("events.avg");
playAVG("image.avg");
playAVG("avg.avg");
playAVG("noavg.avg");
playAVG("noxml.avg");
playAVG("video.avg");

