//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft shared
// source or premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license agreement,
// you are not authorized to use this source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the SOURCE.RTF on your install media or the root of your tools installation.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

///////////////////////////////////////////////////////////////////////////////
// heper to get a designer property as a bool
///////////////////////////////////////////////////////////////////////////////
function getDesignerPropAsBool(tname) {
    if (document.designerProps.hasTrait(tname))
        return document.designerProps.getTrait(tname).value;

    return false;
}

function getSelectionMode() {
    if (getDesignerPropAsBool("usePivot"))
        return 0; // default to object mode when using pivot
    if (document.designerProps.hasTrait("SelectionMode"))
        return document.designerProps.getTrait("SelectionMode").value;
    return 0;
}

function getCommandState(commandName) {
    var commandData = services.commands.getCommandData(commandName);
    if (commandData != null) {
        var trait = commandData.getTrait("state");
        if (trait != null) {
            return trait.value;
        }
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Button state trait
///////////////////////////////////////////////////////////////////////////////
var state = command.getTrait("state");

///////////////////////////////////////////////////////////////////////////////
// Property window and tool option settings 
///////////////////////////////////////////////////////////////////////////////

var enablePropertyWindow = 8;

var stepAmount = 1.0;

function StepAmountChanged(sender, args) {
    stepAmount = document.toolProps.getTrait("StepAmount").value;
}

var toolProps;
var toolPropCookie;
var snapCookie;
function createOptions() {
    toolProps = document.createElement("toolProps", "type", "toolProps");
    toolProps.getOrCreateTrait("StepAmount", "float", enablePropertyWindow);
    document.toolProps = toolProps;

    var snapTrait = document.designerProps.getOrCreateTrait("snap", "bool", 0);
    snapCookie = snapTrait.addHandler("OnDataChanged", OnSnapEnabledTraitChanged);

    toolProps.getTrait("StepAmount").value = stepAmount;

    // Set up the callback when the option traits are changed
    toolPropCookie = toolProps.getTrait("StepAmount").addHandler("OnDataChanged", StepAmountChanged);

    OnSnapEnabledTraitChanged(null, null);
}

function OnSnapEnabledTraitChanged(sender, args) {
    var snapTrait = document.designerProps.getOrCreateTrait("snap", "bool", 0);
    if (toolProps != null) {
        var stepAmountTrait = toolProps.getTrait("StepAmount");
        if (stepAmountTrait != null) {
            var newFlags = stepAmountTrait.flags;
            if (snapTrait.value) {
                newFlags |= enablePropertyWindow;
            }
            else {
                newFlags &= ~enablePropertyWindow;
            }
            stepAmountTrait.flags = newFlags;

            document.refreshPropertyWindow();
        }
    }
}

function getCameraElement()
{
    var camera = document.elements.findElementByTypeId("Microsoft.VisualStudio.3D.PerspectiveCamera");
    return camera;
}

function getWorldMatrix(element) {
    return element.getTrait("WorldTransform").value;
}

// find the mesh child
function findFirstChildMesh(parent) {
    // find the mesh child
    for (var i = 0; i < parent.childCount; i++) {

        // get child and its materials
        var child = parent.getChild(i);
        if (child.typeId == "Microsoft.VisualStudio.3D.Mesh") {
            return child;
        }
    }
    return null;
}

function getFirstSelectedWithoutAncestorInSelection() {
    var count = services.selection.count;
    for (var i = 0; i < count; i++) {
        var currSelected = services.selection.getElement(i);

        //
        // don't operate on items whose parents (in scene) are ancestors
        // since this will double the amount of translation applied to those
        //
        var hasAncestor = false;
        for (var otherIndex = 0; otherIndex < count; otherIndex++) {
            if (otherIndex != i) {
                var ancestor = services.selection.getElement(otherIndex);
                if (currSelected.behavior.isAncestor(ancestor)) {
                    hasAncestor = true;
                    break;
                }
            }
        }

        if (!hasAncestor) {
            return currSelected;
        }
    }
    return null;
}

///////////////////////////////////////////////////////////////////////////////
// Manipulator registration and event handling
///////////////////////////////////////////////////////////////////////////////
var manipulatorData = services.manipulators.getManipulatorData("ScaleManipulator");
var manipulator = services.manipulators.getManipulator("ScaleManipulator");
var undoableItem;

var manipulatorTraitXYZTraitChangedCookie;
var mxyz;

var accumDx;
var accumDy;
var accumDz;

///////////////////////////////////////////////////////////////////////////////
// Scale logic
///////////////////////////////////////////////////////////////////////////////
function coreScale(dx, dy, dz) {

    var selectionMode = getSelectionMode();

    var selectedElement = getFirstSelectedWithoutAncestorInSelection();

    if (selectedElement == null) {
        return;
    }

    if (selectionMode == 0) {

        //
        // object mode
        //
        var t = selectedElement.getTrait("Scale").value;

        var isSnapMode = getDesignerPropAsBool("snap");
        if (isSnapMode && stepAmount != 0) {

            var targetX = t[0] + dx + accumDx;
            var targetY = t[1] + dy + accumDy;
            var targetZ = t[2] + dz + accumDz;

            var roundedX = Math.round(targetX / stepAmount) * stepAmount;
            var roundedY = Math.round(targetY / stepAmount) * stepAmount;
            var roundedZ = Math.round(targetZ / stepAmount) * stepAmount;

            var halfStep = stepAmount * 0.5;
            var stepPct = halfStep * 0.9;

            if (Math.abs(roundedX - targetX) < stepPct) {
                t[0] = roundedX;
            }

            if (Math.abs(roundedY - targetY) < stepPct) {
                t[1] = roundedY;
            }

            if (Math.abs(roundedZ - targetZ) < stepPct) {
                t[2] = roundedZ;
            }

            accumDx = targetX - t[0];
            accumDy = targetY - t[1];
            accumDz = targetZ - t[2];
        }
        else {
            t[0] = t[0] + dx;
            t[1] = t[1] + dy;
            t[2] = t[2] + dz;
        }

        var minScale = 0.00001;
        if (Math.abs(t[0]) < minScale) {
            t[0] = minScale;
        }
        if (Math.abs(t[1]) < minScale) {
            t[1] = minScale;
        }
        if (Math.abs(t[2]) < minScale) {
            t[2] = minScale;
        }

        undoableItem._lastValue = t;
        undoableItem.onDo();
    }
    else if (selectionMode == 1 || selectionMode == 2 || selectionMode == 3) {

        // subobjects    
        undoableItem._currentDelta[0] = dx;
        undoableItem._currentDelta[1] = dy;
        undoableItem._currentDelta[2] = dz;

        undoableItem.onDo();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Listens to manipulator position changes
//
///////////////////////////////////////////////////////////////////////////////
function onManipulatorXYZChangedHandler(sender, args) {
    var xyzDelta = manipulatorData.getTrait("ManipulatorTraitXYZ").value;
    var dx = xyzDelta[0];
    var dy = xyzDelta[1];
    var dz = xyzDelta[2];

    coreScale(dx, dy, dz);
}

function UndoableSubobjectScale(obj, elem) {

    obj._totalDelta = [1, 1, 1];
    obj._currentDelta = [0, 0, 0];
    
    // find the mesh child
    obj._meshElem = findFirstChildMesh(elem);
    if (obj._meshElem == null) {
        return;
    }

    // get the scale origin in local space of node we're manipulating
    var manipulatorToWorld = manipulator.getWorldTransform();
    manipulatorToWorld = math.getNormalizedMatrix(manipulatorToWorld);

    var localToWorldMatrix = getWorldMatrix(obj._meshElem);

    var worldToLocal = math.getInverse(localToWorldMatrix);
    obj._manipulatorToLocal = math.multiplyMatrix(worldToLocal, manipulatorToWorld);
    obj._localToManipulator = math.getInverse(obj._manipulatorToLocal);

    obj._mesh = obj._meshElem.behavior;

    // loop over the elements in the polygon collection
    var collElem = obj._mesh.selectedObjects;
    if (collElem == null) {
        return;
    }

    obj._collectionElem = collElem.clone();

    // get the actual collection we can operate on
    obj._collection = obj._collectionElem.behavior;

    obj._geom = obj._meshElem.getTrait("Geometry").value
}

function SubobjectDoScale(obj)
{
    var polygonPoints = obj.getPoints();
    var lastTotal = [0, 0, 0];

    lastTotal[0] = obj._totalDelta[0];
    lastTotal[1] = obj._totalDelta[1];
    lastTotal[2] = obj._totalDelta[2];

    obj._totalDelta[0] += obj._currentDelta[0];
    obj._totalDelta[1] += obj._currentDelta[1];
    obj._totalDelta[2] += obj._currentDelta[2];

    var scale = [obj._totalDelta[0] / lastTotal[0], obj._totalDelta[1] / lastTotal[1], obj._totalDelta[2] / lastTotal[2]];

    var scaleMatrix = math.createScale(scale);

    var transform = math.multiplyMatrix(scaleMatrix, obj._localToManipulator);
    transform = math.multiplyMatrix(obj._manipulatorToLocal, transform);

    // loop over the unique set of indices and transform the associated point
    for (var key in polygonPoints) {

        var ptIdx = polygonPoints[key];
        var pt = obj._geom.getPointAt(ptIdx);

        pt = math.transformPoint(transform, pt);

        obj._geom.setPointAt(ptIdx, pt);
    }

    // invalidate the mesh collision
    obj._mesh.recomputeCachedGeometry();
}

function SubobjectUndoScale(obj) {
    var polygonPoints = obj.getPoints();

    var scale = [1.0 / obj._totalDelta[0], 1.0 / obj._totalDelta[1], 1.0 / obj._totalDelta[2]];

    var scaleMatrix = math.createScale(scale);

    var transform = math.multiplyMatrix(scaleMatrix, obj._localToManipulator);
    transform = math.multiplyMatrix(obj._manipulatorToLocal, transform);

    // loop over the unique set of indices and transform the associated point
    for (var key in polygonPoints) {

        var ptIdx = polygonPoints[key];
        var pt = obj._geom.getPointAt(ptIdx);

        pt = math.transformPoint(transform, pt);

        obj._geom.setPointAt(ptIdx, pt);
    }

    obj._currentDelta[0] = obj._totalDelta[0] - 1;
    obj._currentDelta[1] = obj._totalDelta[1] - 1;
    obj._currentDelta[2] = obj._totalDelta[2] - 1;

    obj._totalDelta[0] = 1;
    obj._totalDelta[1] = 1;
    obj._totalDelta[2] = 1;

    obj._mesh.recomputeCachedGeometry();
}

function onBeginManipulation() {

    undoableItem = null;

    //
    // Check the selection mode
    //
    var selectionMode = getSelectionMode();
    if (selectionMode == 0) {
        //
        // object selection
        //

        accumDx = 0;
        accumDy = 0;
        accumDz = 0;

        function UndoableScale(trait, traitValues, initialValue) {
            this._traitArray = traitArray;
            this._traitValues = traitValues;
            this._initialValues = initialValue;
        }

        var traitArray = new Array();
        var traitValues = new Array();
        var initialValues = new Array();

        //
        // add the traits of selected items to the collections that we'll be operating on
        //
        var count = services.selection.count;
        for (i = 0; i < count; i++) {
            var currSelected = services.selection.getElement(i);

            //
            // don't operate on items whose parents (in scene) are ancestors
            // since this will double the amount of translation applied to those
            //
            var hasAncestor = false;
            for (var otherIndex = 0; otherIndex < count; otherIndex++) {
                if (otherIndex != i) {
                    var ancestor = services.selection.getElement(otherIndex);
                    if (currSelected.behavior.isAncestor(ancestor)) {
                        hasAncestor = true;
                        break;
                    }
                }
            }

            if (!hasAncestor) {
                var currTrait = currSelected.getTrait("Scale");

                traitArray.push(currTrait);
                traitValues.push(currTrait.value);
                initialValues.push(currTrait.value);
            }
        }


        // create the undoable item
        undoableItem = new UndoableScale(traitArray, traitValues, initialValues);

        undoableItem.onDo = function () {

            var count = this._traitArray.length;

            // movement delta of all the selected is determined by delta of the first selected
            var delta = [0, 0, 0];
            if (count > 0) {
                delta[0] = this._lastValue[0] - this._initialValues[0][0];
                delta[1] = this._lastValue[1] - this._initialValues[0][1];
                delta[2] = this._lastValue[2] - this._initialValues[0][2];
            }

            for (i = 0; i < count; i++) {
                var currTrait = this._traitArray[i];
                this._traitValues[i][0] = this._initialValues[i][0] + delta[0];
                this._traitValues[i][1] = this._initialValues[i][1] + delta[1];
                this._traitValues[i][2] = this._initialValues[i][2] + delta[2];

                var theVal = [this._traitValues[i][0], this._traitValues[i][1], this._traitValues[i][2]];
                this._traitArray[i].value = theVal;
            }
        }

        undoableItem.onUndo = function () {
            var count = this._traitArray.length;
            for (i = 0; i < count; i++) {
                this._traitArray[i].value = this._initialValues[i];
            }
        }
    }
    else {
        
        // create the undoable item
        undoableItem = new Object();
        UndoableSubobjectScale(undoableItem, document.selectedElement);

        if (selectionMode == 1) {
            
            // face selection mode

            // gets the points
            undoableItem.getPoints = function () {

                // map we will store indices in
                // we use the map instead of array to eliminate dups
                var polygonPoints = new Object();

                // loop over the point indices in the poly collection
                var polyCount = this._collection.getPolygonCount();
                for (var i = 0; i < polyCount; i++) {
                    var polyIndex = this._collection.getPolygon(i);

                    // get the point count and loop over polygon points
                    var polygonPointCount = this._geom.getPolygonPointCount(polyIndex);
                    for (var j = 0; j < polygonPointCount; j++) {

                        // get the point index
                        var pointIndex = this._geom.getPolygonPoint(polyIndex, j);
                        polygonPoints[pointIndex] = pointIndex;
                    }
                }

                return polygonPoints;
            }
        }
        else if (selectionMode == 2) {

            // edges selection mode

            // gets the points
            undoableItem.getPoints = function () {

                // we use the map instead of array to eliminate dups
                var polygonPoints = new Object();

                // loop over the edges collection
                var edgeCount = this._collection.getEdgeCount();
                for (var i = 0; i < edgeCount; i++) {
                    var edge = this._collection.getEdge(i);
                    polygonPoints[edge[0]] = edge[0];
                    polygonPoints[edge[1]] = edge[1];
                }

                return polygonPoints;
            }
        }
        else if (selectionMode == 3) {

            // point selection mode

            // gets the points
            undoableItem.getPoints = function () {

                // we use the map instead of array to eliminate dups
                var polygonPoints = new Object();

                // loop over the point indices in the collection
                var ptCount = this._collection.getPointCount();
                for (var i = 0; i < ptCount; i++) {
                    var pt = this._collection.getPoint(i);
                    polygonPoints[pt] = pt;
                }

                return polygonPoints;
            }
        }

        //
        // do
        //
        undoableItem.onDo = function () {
            SubobjectDoScale(this);
        }

        //
        // undo
        //
        undoableItem.onUndo = function () {
            SubobjectUndoScale(this);
        }
    }

    if (undoableItem != null) {
        //
        // getName()
        //
        undoableItem.getName = function () {
            var IDS_MreUndoScale = 145;
            return services.strings.getStringFromId(IDS_MreUndoScale);
        }

        // add to undo stack
        services.undoService.addUndoableItem(undoableItem);
    }
}

//
// the tool
//
var tool = new Object();

var onBeginManipulationHandler;

tool.activate = function () {
    state.value = 2;

    createOptions();

    services.manipulators.activate("ScaleManipulator")

    mxyz = manipulatorData.getTrait("ManipulatorTraitXYZ");

    manipulatorTraitXYZTraitChangedCookie = mxyz.addHandler("OnDataChanged", onManipulatorXYZChangedHandler);

    onBeginManipulationHandler = manipulator.addHandler("OnBeginManipulation", onBeginManipulation);
}

tool.deactivate = function () {
    state.value = 0;

    toolProps.getTrait("StepAmount").removeHandler("OnDataChanged", toolPropCookie);

    var snapTrait = document.designerProps.getOrCreateTrait("snap", "bool", 0);
    snapTrait.removeHandler("OnDataChanged", snapCookie);

    mxyz.removeHandler("OnDataChanged", manipulatorTraitXYZTraitChangedCookie);
    
    manipulator.removeHandler("OnBeginManipulation", onBeginManipulationHandler);

    services.manipulators.deactivate("ScaleManipulator");
}

///////////////////////////////////////////////////////////////////////////////
// Global code
///////////////////////////////////////////////////////////////////////////////
if (state.value != 2) {
    document.setTool(tool);
}
// SIG // Begin signature block
// SIG // MIIjhAYJKoZIhvcNAQcCoIIjdTCCI3ECAQExDzANBglg
// SIG // hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
// SIG // BgEEAYI3AgEeMCQCAQEEEBDgyQbOONQRoqMAEEvTUJAC
// SIG // AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
// SIG // sfppIvAUfmoyDEnMCUyCiCNyBUQJN01CKjbrhRDg+Iqg
// SIG // gg2BMIIF/zCCA+egAwIBAgITMwAAAd9r8C6Sp0q00AAA
// SIG // AAAB3zANBgkqhkiG9w0BAQsFADB+MQswCQYDVQQGEwJV
// SIG // UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
// SIG // UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
// SIG // cmF0aW9uMSgwJgYDVQQDEx9NaWNyb3NvZnQgQ29kZSBT
// SIG // aWduaW5nIFBDQSAyMDExMB4XDTIwMTIxNTIxMzE0NVoX
// SIG // DTIxMTIwMjIxMzE0NVowdDELMAkGA1UEBhMCVVMxEzAR
// SIG // BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
// SIG // bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
// SIG // bjEeMBwGA1UEAxMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
// SIG // MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA
// SIG // trsZWRAAo6nx5LhcqAsHy9uaHyPQ2VireMBI9yQUOPBj
// SIG // 7dVLA7/N+AnKFFDzJ7P+grT6GkOE4cv5GzjoP8yQJ6yX
// SIG // ojEKkXti7HW/zUiNoF11/ZWndf8j1Azl6OBjcD416tSW
// SIG // Yvh2VfdW1K+mY83j49YPm3qbKnfxwtV0nI9H092gMS0c
// SIG // pCUsxMRAZlPXksrjsFLqvgq4rnULVhjHSVOudL/yps3z
// SIG // OOmOpaPzAp56b898xC+zzHVHcKo/52IRht1FSC8V+7QH
// SIG // TG8+yzfuljiKU9QONa8GqDlZ7/vFGveB8IY2ZrtUu98n
// SIG // le0WWTcaIRHoCYvWGLLF2u1GVFJAggPipwIDAQABo4IB
// SIG // fjCCAXowHwYDVR0lBBgwFgYKKwYBBAGCN0wIAQYIKwYB
// SIG // BQUHAwMwHQYDVR0OBBYEFDj2zC/CHZDRrQnzJlT7byOl
// SIG // WfPjMFAGA1UdEQRJMEekRTBDMSkwJwYDVQQLEyBNaWNy
// SIG // b3NvZnQgT3BlcmF0aW9ucyBQdWVydG8gUmljbzEWMBQG
// SIG // A1UEBRMNMjMwMDEyKzQ2MzAwOTAfBgNVHSMEGDAWgBRI
// SIG // bmTlUAXTgqoXNzcitW2oynUClTBUBgNVHR8ETTBLMEmg
// SIG // R6BFhkNodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtp
// SIG // b3BzL2NybC9NaWNDb2RTaWdQQ0EyMDExXzIwMTEtMDct
// SIG // MDguY3JsMGEGCCsGAQUFBwEBBFUwUzBRBggrBgEFBQcw
// SIG // AoZFaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9w
// SIG // cy9jZXJ0cy9NaWNDb2RTaWdQQ0EyMDExXzIwMTEtMDct
// SIG // MDguY3J0MAwGA1UdEwEB/wQCMAAwDQYJKoZIhvcNAQEL
// SIG // BQADggIBAJ56h7Q8mFBWlQJLwCtHqqup4aC/eUmULt0Z
// SIG // 6We7XUPPUEd/vuwPuIa6+1eMcZpAeQTm0tGCvjACxNNm
// SIG // rY8FoD3aWEOvFnSxq6CWR5G2XYBERvu7RExZd2iheCqa
// SIG // EmhjrJGV6Uz5wmjKNj16ADFTBqbEBELMIpmatyEN50UH
// SIG // wZSdD6DDHDf/j5LPGUy9QaD2LCaaJLenKpefaugsqWWC
// SIG // MIMifPdh6bbcmxyoNWbUC1JUl3HETJboD4BHDWSWoDxI
// SIG // D2J4uG9dbJ40QIH9HckNMyPWi16k8VlFOaQiBYj09G9s
// SIG // LMc0agrchqqZBjPD/RmszvHmqJlSLQmAXCUgcgcf6UtH
// SIG // EmMAQRwGcSTg1KsUl6Ehg75k36lCV57Z1pC+KJKJNRYg
// SIG // g2eI6clzkLp2+noCF75IEO429rjtujsNJvEcJXg74TjK
// SIG // 5x7LqYjj26Myq6EmuqWhbVUofPWm1EqKEfEHWXInppqB
// SIG // YXFpBMBYOLKc72DT+JyLNfd9utVsk2kTGaHHhrp+xgk9
// SIG // kZeud7lI/hfoPeHOtwIc0quJIXS+B5RSD9nj79vbJn1J
// SIG // x7RqusmBQy509Kv2Pg4t48JaBfBFpJB0bUrl5RVG05sK
// SIG // /5Qw4G6WYioS0uwgUw499iNC+Yud9vrh3M8PNqGQ5mJm
// SIG // JiFEjG2ToEuuYe/e64+SSejpHhFCaAFcMIIHejCCBWKg
// SIG // AwIBAgIKYQ6Q0gAAAAAAAzANBgkqhkiG9w0BAQsFADCB
// SIG // iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
// SIG // b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
// SIG // Y3Jvc29mdCBDb3Jwb3JhdGlvbjEyMDAGA1UEAxMpTWlj
// SIG // cm9zb2Z0IFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5
// SIG // IDIwMTEwHhcNMTEwNzA4MjA1OTA5WhcNMjYwNzA4MjEw
// SIG // OTA5WjB+MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSgwJgYDVQQD
// SIG // Ex9NaWNyb3NvZnQgQ29kZSBTaWduaW5nIFBDQSAyMDEx
// SIG // MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA
// SIG // q/D6chAcLq3YbqqCEE00uvK2WCGfQhsqa+laUKq4Bjga
// SIG // BEm6f8MMHt03a8YS2AvwOMKZBrDIOdUBFDFC04kNeWSH
// SIG // fpRgJGyvnkmc6Whe0t+bU7IKLMOv2akrrnoJr9eWWcpg
// SIG // GgXpZnboMlImEi/nqwhQz7NEt13YxC4Ddato88tt8zpc
// SIG // oRb0RrrgOGSsbmQ1eKagYw8t00CT+OPeBw3VXHmlSSnn
// SIG // Db6gE3e+lD3v++MrWhAfTVYoonpy4BI6t0le2O3tQ5GD
// SIG // 2Xuye4Yb2T6xjF3oiU+EGvKhL1nkkDstrjNYxbc+/jLT
// SIG // swM9sbKvkjh+0p2ALPVOVpEhNSXDOW5kf1O6nA+tGSOE
// SIG // y/S6A4aN91/w0FK/jJSHvMAhdCVfGCi2zCcoOCWYOUo2
// SIG // z3yxkq4cI6epZuxhH2rhKEmdX4jiJV3TIUs+UsS1Vz8k
// SIG // A/DRelsv1SPjcF0PUUZ3s/gA4bysAoJf28AVs70b1FVL
// SIG // 5zmhD+kjSbwYuER8ReTBw3J64HLnJN+/RpnF78IcV9uD
// SIG // jexNSTCnq47f7Fufr/zdsGbiwZeBe+3W7UvnSSmnEyim
// SIG // p31ngOaKYnhfsi+E11ecXL93KCjx7W3DKI8sj0A3T8Hh
// SIG // hUSJxAlMxdSlQy90lfdu+HggWCwTXWCVmj5PM4TasIgX
// SIG // 3p5O9JawvEagbJjS4NaIjAsCAwEAAaOCAe0wggHpMBAG
// SIG // CSsGAQQBgjcVAQQDAgEAMB0GA1UdDgQWBBRIbmTlUAXT
// SIG // gqoXNzcitW2oynUClTAZBgkrBgEEAYI3FAIEDB4KAFMA
// SIG // dQBiAEMAQTALBgNVHQ8EBAMCAYYwDwYDVR0TAQH/BAUw
// SIG // AwEB/zAfBgNVHSMEGDAWgBRyLToCMZBDuRQFTuHqp8cx
// SIG // 0SOJNDBaBgNVHR8EUzBRME+gTaBLhklodHRwOi8vY3Js
// SIG // Lm1pY3Jvc29mdC5jb20vcGtpL2NybC9wcm9kdWN0cy9N
// SIG // aWNSb29DZXJBdXQyMDExXzIwMTFfMDNfMjIuY3JsMF4G
// SIG // CCsGAQUFBwEBBFIwUDBOBggrBgEFBQcwAoZCaHR0cDov
// SIG // L3d3dy5taWNyb3NvZnQuY29tL3BraS9jZXJ0cy9NaWNS
// SIG // b29DZXJBdXQyMDExXzIwMTFfMDNfMjIuY3J0MIGfBgNV
// SIG // HSAEgZcwgZQwgZEGCSsGAQQBgjcuAzCBgzA/BggrBgEF
// SIG // BQcCARYzaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3Br
// SIG // aW9wcy9kb2NzL3ByaW1hcnljcHMuaHRtMEAGCCsGAQUF
// SIG // BwICMDQeMiAdAEwAZQBnAGEAbABfAHAAbwBsAGkAYwB5
// SIG // AF8AcwB0AGEAdABlAG0AZQBuAHQALiAdMA0GCSqGSIb3
// SIG // DQEBCwUAA4ICAQBn8oalmOBUeRou09h0ZyKbC5YR4WOS
// SIG // mUKWfdJ5DJDBZV8uLD74w3LRbYP+vj/oCso7v0epo/Np
// SIG // 22O/IjWll11lhJB9i0ZQVdgMknzSGksc8zxCi1LQsP1r
// SIG // 4z4HLimb5j0bpdS1HXeUOeLpZMlEPXh6I/MTfaaQdION
// SIG // 9MsmAkYqwooQu6SpBQyb7Wj6aC6VoCo/KmtYSWMfCWlu
// SIG // WpiW5IP0wI/zRive/DvQvTXvbiWu5a8n7dDd8w6vmSiX
// SIG // mE0OPQvyCInWH8MyGOLwxS3OW560STkKxgrCxq2u5bLZ
// SIG // 2xWIUUVYODJxJxp/sfQn+N4sOiBpmLJZiWhub6e3dMNA
// SIG // BQamASooPoI/E01mC8CzTfXhj38cbxV9Rad25UAqZaPD
// SIG // XVJihsMdYzaXht/a8/jyFqGaJ+HNpZfQ7l1jQeNbB5yH
// SIG // PgZ3BtEGsXUfFL5hYbXw3MYbBL7fQccOKO7eZS/sl/ah
// SIG // XJbYANahRr1Z85elCUtIEJmAH9AAKcWxm6U/RXceNcbS
// SIG // oqKfenoi+kiVH6v7RyOA9Z74v2u3S5fi63V4GuzqN5l5
// SIG // GEv/1rMjaHXmr/r8i+sLgOppO6/8MO0ETI7f33VtY5E9
// SIG // 0Z1WTk+/gFcioXgRMiF670EKsT/7qMykXcGhiJtXcVZO
// SIG // SEXAQsmbdlsKgEhr/Xmfwb1tbWrJUnMTDXpQzTGCFVsw
// SIG // ghVXAgEBMIGVMH4xCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKDAm
// SIG // BgNVBAMTH01pY3Jvc29mdCBDb2RlIFNpZ25pbmcgUENB
// SIG // IDIwMTECEzMAAAHfa/AukqdKtNAAAAAAAd8wDQYJYIZI
// SIG // AWUDBAIBBQCgga4wGQYJKoZIhvcNAQkDMQwGCisGAQQB
// SIG // gjcCAQQwHAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcC
// SIG // ARUwLwYJKoZIhvcNAQkEMSIEIA3LxYYk7s1pD9NgVMjw
// SIG // PnpZ8UaO8AV3Dsem8z8DA7mhMEIGCisGAQQBgjcCAQwx
// SIG // NDAyoBSAEgBNAGkAYwByAG8AcwBvAGYAdKEagBhodHRw
// SIG // Oi8vd3d3Lm1pY3Jvc29mdC5jb20wDQYJKoZIhvcNAQEB
// SIG // BQAEggEAPTo0IBp3ND89WeyYp/bD+566dpxyMp7CmRD7
// SIG // bjDPBwjZsUDMe8gdhlBEwhvbSqcqzcIQG9g8z7kOa4sj
// SIG // rMS7cUxZxV8QbYLJ+YZBkhH7dq9ngr2F7bf03BHKN629
// SIG // LgSZQP73zqsYZcQjVmKmeQS28N5W5A877od7xewlKqqn
// SIG // jHKmD96LOoyA4CbgPErlTW9Qm0No2LETF9ZrFThTtZ16
// SIG // a3FLVGUMmISh8bhsrPGDYplA+dcTrDEqF/vBU1fof/rH
// SIG // i4t9PBDVJv8hteGBDyok7Xmw0SgLeT9sFyG+6mk+tQ86
// SIG // 0fXCPP1/CgPJZ8x/4/agiaO/8t74kzCdyHTGoVE9w6GC
// SIG // EuUwghLhBgorBgEEAYI3AwMBMYIS0TCCEs0GCSqGSIb3
// SIG // DQEHAqCCEr4wghK6AgEDMQ8wDQYJYIZIAWUDBAIBBQAw
// SIG // ggFRBgsqhkiG9w0BCRABBKCCAUAEggE8MIIBOAIBAQYK
// SIG // KwYBBAGEWQoDATAxMA0GCWCGSAFlAwQCAQUABCBtSJ7l
// SIG // Z4y6mVZ6hrI8SWzTJWfxZ56fx6/n6OqyonrC9AIGYK6a
// SIG // tdqWGBMyMDIxMDYxMTAwMjUzMi4xNzJaMASAAgH0oIHQ
// SIG // pIHNMIHKMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSUwIwYDVQQL
// SIG // ExxNaWNyb3NvZnQgQW1lcmljYSBPcGVyYXRpb25zMSYw
// SIG // JAYDVQQLEx1UaGFsZXMgVFNTIEVTTjoxMkJDLUUzQUUt
// SIG // NzRFQjElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUtU3Rh
// SIG // bXAgU2VydmljZaCCDjwwggTxMIID2aADAgECAhMzAAAB
// SIG // U9KCckVsV+OLAAAAAAFTMA0GCSqGSIb3DQEBCwUAMHwx
// SIG // CzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9u
// SIG // MRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNy
// SIG // b3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jv
// SIG // c29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMB4XDTIwMTEx
// SIG // MjE4MjYwNVoXDTIyMDIxMTE4MjYwNVowgcoxCzAJBgNV
// SIG // BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
// SIG // VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
// SIG // Q29ycG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBB
// SIG // bWVyaWNhIE9wZXJhdGlvbnMxJjAkBgNVBAsTHVRoYWxl
// SIG // cyBUU1MgRVNOOjEyQkMtRTNBRS03NEVCMSUwIwYDVQQD
// SIG // ExxNaWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNlMIIB
// SIG // IjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAteu2
// SIG // dAezG/mzQgLlwuhBibBb4FQeot9P45eIJrVMKEBuc91q
// SIG // cUBzSY/rdiOH6RE0EC6MYqB0okfp2yRUZ091go3OlMGx
// SIG // gTJ0CYM4M1V6H1TxyAopomDRIh8vOj7whKVk/L2+o3xX
// SIG // GjlaDHvuMDdjam5TlgOKOsJhEFBZq26SmQfnEVSRe1hN
// SIG // S+jQ30YjxPV0X0ZPs82XfjCYbN3lka+41XRtdL1RJVd4
// SIG // atGBc34aO4Lj9y9GzMYpEct6T3uFuoO0MH+THMQ6TNAG
// SIG // rc/QjZLHAD8oeI2ThdH+3Yi9QRhE/3OL6fIkE5XyqhIQ
// SIG // m7dxYvx6ueHMF4xNWlMc0L2uW/3m7QIDAQABo4IBGzCC
// SIG // ARcwHQYDVR0OBBYEFMaii3NIKqmzMBlAt+BwbawxVkR1
// SIG // MB8GA1UdIwQYMBaAFNVjOlyKMZDzQ3t8RhvFM2hahW1V
// SIG // MFYGA1UdHwRPME0wS6BJoEeGRWh0dHA6Ly9jcmwubWlj
// SIG // cm9zb2Z0LmNvbS9wa2kvY3JsL3Byb2R1Y3RzL01pY1Rp
// SIG // bVN0YVBDQV8yMDEwLTA3LTAxLmNybDBaBggrBgEFBQcB
// SIG // AQROMEwwSgYIKwYBBQUHMAKGPmh0dHA6Ly93d3cubWlj
// SIG // cm9zb2Z0LmNvbS9wa2kvY2VydHMvTWljVGltU3RhUENB
// SIG // XzIwMTAtMDctMDEuY3J0MAwGA1UdEwEB/wQCMAAwEwYD
// SIG // VR0lBAwwCgYIKwYBBQUHAwgwDQYJKoZIhvcNAQELBQAD
// SIG // ggEBAJRovXl8+iBEPscBc0miUdycR7GonoMpvQ7EV3KX
// SIG // 3e365U1Gf+YlZMhavGF8iqBCPu21TsyZU39zxfFHWAsu
// SIG // MrFmnurMyPm7M9vW6GKEZcEmsXUDoPYcJV9TJH3TJ4Sf
// SIG // iIsN+RFWKe6zFY3TfYummR5XSj/zw0DbvrsxB6GyjS6W
// SIG // bMXviyP4HcbiIhSQztzvXk7p9O8Vh34ZyMSdsEu8uTxB
// SIG // 6XcWU0F9p71kdG43Zrea+ocuV9LDQ9V2YmlNmqZdVm/R
// SIG // 2tm5zuAPJa3u8GY0vovFwVVSWhOC15nExXkJZWZXb6vM
// SIG // gZrw6OML4xThVXJKsgUqQjDCaUas5KHRg8SdOjQwggZx
// SIG // MIIEWaADAgECAgphCYEqAAAAAAACMA0GCSqGSIb3DQEB
// SIG // CwUAMIGIMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMTIwMAYDVQQD
// SIG // EylNaWNyb3NvZnQgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRo
// SIG // b3JpdHkgMjAxMDAeFw0xMDA3MDEyMTM2NTVaFw0yNTA3
// SIG // MDEyMTQ2NTVaMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
// SIG // BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
// SIG // MDEwMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC
// SIG // AQEAqR0NvHcRijog7PwTl/X6f2mUa3RUENWlCgCChfvt
// SIG // fGhLLF/Fw+Vhwna3PmYrW/AVUycEMR9BGxqVHc4JE458
// SIG // YTBZsTBED/FgiIRUQwzXTbg4CLNC3ZOs1nMwVyaCo0UN
// SIG // 0Or1R4HNvyRgMlhgRvJYR4YyhB50YWeRX4FUsc+TTJLB
// SIG // xKZd0WETbijGGvmGgLvfYfxGwScdJGcSchohiq9LZIlQ
// SIG // YrFd/XcfPfBXday9ikJNQFHRD5wGPmd/9WbAA5ZEfu/Q
// SIG // S/1u5ZrKsajyeioKMfDaTgaRtogINeh4HLDpmc085y9E
// SIG // uqf03GS9pAHBIAmTeM38vMDJRF1eFpwBBU8iTQIDAQAB
// SIG // o4IB5jCCAeIwEAYJKwYBBAGCNxUBBAMCAQAwHQYDVR0O
// SIG // BBYEFNVjOlyKMZDzQ3t8RhvFM2hahW1VMBkGCSsGAQQB
// SIG // gjcUAgQMHgoAUwB1AGIAQwBBMAsGA1UdDwQEAwIBhjAP
// SIG // BgNVHRMBAf8EBTADAQH/MB8GA1UdIwQYMBaAFNX2VsuP
// SIG // 6KJcYmjRPZSQW9fOmhjEMFYGA1UdHwRPME0wS6BJoEeG
// SIG // RWh0dHA6Ly9jcmwubWljcm9zb2Z0LmNvbS9wa2kvY3Js
// SIG // L3Byb2R1Y3RzL01pY1Jvb0NlckF1dF8yMDEwLTA2LTIz
// SIG // LmNybDBaBggrBgEFBQcBAQROMEwwSgYIKwYBBQUHMAKG
// SIG // Pmh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2kvY2Vy
// SIG // dHMvTWljUm9vQ2VyQXV0XzIwMTAtMDYtMjMuY3J0MIGg
// SIG // BgNVHSABAf8EgZUwgZIwgY8GCSsGAQQBgjcuAzCBgTA9
// SIG // BggrBgEFBQcCARYxaHR0cDovL3d3dy5taWNyb3NvZnQu
// SIG // Y29tL1BLSS9kb2NzL0NQUy9kZWZhdWx0Lmh0bTBABggr
// SIG // BgEFBQcCAjA0HjIgHQBMAGUAZwBhAGwAXwBQAG8AbABp
// SIG // AGMAeQBfAFMAdABhAHQAZQBtAGUAbgB0AC4gHTANBgkq
// SIG // hkiG9w0BAQsFAAOCAgEAB+aIUQ3ixuCYP4FxAz2do6Eh
// SIG // b7Prpsz1Mb7PBeKp/vpXbRkws8LFZslq3/Xn8Hi9x6ie
// SIG // JeP5vO1rVFcIK1GCRBL7uVOMzPRgEop2zEBAQZvcXBf/
// SIG // XPleFzWYJFZLdO9CEMivv3/Gf/I3fVo/HPKZeUqRUgCv
// SIG // OA8X9S95gWXZqbVr5MfO9sp6AG9LMEQkIjzP7QOllo9Z
// SIG // Kby2/QThcJ8ySif9Va8v/rbljjO7Yl+a21dA6fHOmWaQ
// SIG // jP9qYn/dxUoLkSbiOewZSnFjnXshbcOco6I8+n99lmqQ
// SIG // eKZt0uGc+R38ONiU9MalCpaGpL2eGq4EQoO4tYCbIjgg
// SIG // tSXlZOz39L9+Y1klD3ouOVd2onGqBooPiRa6YacRy5rY
// SIG // DkeagMXQzafQ732D8OE7cQnfXXSYIghh2rBQHm+98eEA
// SIG // 3+cxB6STOvdlR3jo+KhIq/fecn5ha293qYHLpwmsObvs
// SIG // xsvYgrRyzR30uIUBHoD7G4kqVDmyW9rIDVWZeodzOwjm
// SIG // mC3qjeAzLhIp9cAvVCch98isTtoouLGp25ayp0Kiyc8Z
// SIG // QU3ghvkqmqMRZjDTu3QyS99je/WZii8bxyGvWbWu3EQ8
// SIG // l1Bx16HSxVXjad5XwdHeMMD9zOZN+w2/XU/pnR4ZOC+8
// SIG // z1gFLu8NoFA12u8JJxzVs341Hgi62jbb01+P3nSISRKh
// SIG // ggLOMIICNwIBATCB+KGB0KSBzTCByjELMAkGA1UEBhMC
// SIG // VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
// SIG // B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
// SIG // b3JhdGlvbjElMCMGA1UECxMcTWljcm9zb2Z0IEFtZXJp
// SIG // Y2EgT3BlcmF0aW9uczEmMCQGA1UECxMdVGhhbGVzIFRT
// SIG // UyBFU046MTJCQy1FM0FFLTc0RUIxJTAjBgNVBAMTHE1p
// SIG // Y3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2WiIwoBATAH
// SIG // BgUrDgMCGgMVAIpKTe3HDtZPTyghgr2kFisiedGboIGD
// SIG // MIGApH4wfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldh
// SIG // c2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNV
// SIG // BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UE
// SIG // AxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAw
// SIG // DQYJKoZIhvcNAQEFBQACBQDkbN8FMCIYDzIwMjEwNjEx
// SIG // MDI1NzA5WhgPMjAyMTA2MTIwMjU3MDlaMHcwPQYKKwYB
// SIG // BAGEWQoEATEvMC0wCgIFAORs3wUCAQAwCgIBAAICCt8C
// SIG // Af8wBwIBAAICEZgwCgIFAORuMIUCAQAwNgYKKwYBBAGE
// SIG // WQoEAjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQACAweh
// SIG // IKEKMAgCAQACAwGGoDANBgkqhkiG9w0BAQUFAAOBgQBt
// SIG // nlXpJN57Fx7eK/hBb+CCrDxvMq+zQRP38spiBqji/ioL
// SIG // y228rYBPFuIvoB46lvoqM4rUa4Q/81a6w86Vm2NoFSiz
// SIG // bgwdveTdT4XWfhN3ayNCpBNz0cC+mGOYhom2vVQmSOkN
// SIG // WOFFDt8a+domVYrHRDeu7kEF7JB9Uo7dAItnGTGCAw0w
// SIG // ggMJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
// SIG // BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
// SIG // MDEwAhMzAAABU9KCckVsV+OLAAAAAAFTMA0GCWCGSAFl
// SIG // AwQCAQUAoIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcN
// SIG // AQkQAQQwLwYJKoZIhvcNAQkEMSIEIDzE2JQbI57vWL2I
// SIG // vWvWbSXIwzEyd4DtKggZP6Eka5zFMIH6BgsqhkiG9w0B
// SIG // CRACLzGB6jCB5zCB5DCBvQQgUMEKjzm0IAgJ/JehkEOm
// SIG // mh0sgxxvbA4fasQFaI21jwAwgZgwgYCkfjB8MQswCQYD
// SIG // VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
// SIG // A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
// SIG // IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQg
// SIG // VGltZS1TdGFtcCBQQ0EgMjAxMAITMwAAAVPSgnJFbFfj
// SIG // iwAAAAABUzAiBCAPZBOwLgBuZm9cDY1QrTz/uQBUVJF7
// SIG // cDmmc6bjN7lJ3zANBgkqhkiG9w0BAQsFAASCAQAGkayq
// SIG // 6YR1M641EvropHwLj0ZeJVECZ0szT63oKnwAdNkKMEH+
// SIG // LA9q6nRB2jK71tFneza1MZiPJN313tIFQcZqyBnP5K+p
// SIG // k4OR3pnvMZK1Ovv5wqSYd2/2R5AszuA0P2otafZwgM1r
// SIG // KXG6ID504Qbonu5WrkwGovJFhGJQZ2wnfjoSgda2JQHJ
// SIG // ACqPoRP0niEFkJp64XKr3XIJ7s4wBcFNi2obR7nd3FR0
// SIG // tVfdG6G50ASPAfjWBH5l13obG7ICeK074ZLzStjjyfvM
// SIG // TVDOMolq47+1CZqwLydMFmfw+Xt9nAd1jZG/cGbr0Ohp
// SIG // KHzMSrCZ9gnpdGBVXI/MlLnsmaU2
// SIG // End signature block
