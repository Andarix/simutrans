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

// services.debug.trace("Rotate");


///////////////////////////////////////////////////////////////////////////////
//
// Global data
//
///////////////////////////////////////////////////////////////////////////////

var useStep = false;
var state = command.getTrait("state");
var stepAmount = 5.0;
var enablePropertyWindow = 8;
var toolProps;

// establish tool options and deal with tool option changes
var manipulatorTraitXYZTraitChangedCookie;

// get manipulator
var manipulatorData = services.manipulators.getManipulatorData("RotationManipulator");
var manipulator = services.manipulators.getManipulator("RotationManipulator");
var undoableItem;

var mxyz = manipulatorData.getTrait("RotationManipulatorTraitXYZ");

var accumDx;
var accumDy;
var accumDz;

var tool = new Object();

var onBeginManipulationHandler;


///////////////////////////////////////////////////////////////////////////////
// designer props bool access
///////////////////////////////////////////////////////////////////////////////
function getDesignerPropAsBool(tname) {
    if (document.designerProps.hasTrait(tname))
        return document.designerProps.getTrait(tname).value;
    return false;
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

function getShouldUsePivot() {
    return getDesignerPropAsBool("usePivot");
}

function getSelectionMode() {
    if (getShouldUsePivot())
        return 0; // default to object mode when using pivot
    if (document.designerProps.hasTrait("SelectionMode"))
        return document.designerProps.getTrait("SelectionMode").value;
    return 0;
}

// setup tool options
function UseStepChanged(sender, args) {
    useStep = document.toolProps.getTrait("UseStep").value;
}

function StepAmountChanged(sender, args) {
    stepAmount = document.toolProps.getTrait("StepAmount").value;
}

var snapCookie;
var toolPropCookie;
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
//
// Helper functions
//
///////////////////////////////////////////////////////////////////////////////

function getWorldMatrix(element) {
    return element.getTrait("WorldTransform").value;
}

function getCameraElement() {
    var camera = document.elements.findElementByTypeId("Microsoft.VisualStudio.3D.PerspectiveCamera");
    return camera;
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

function getRotationTraitId() {
    return "Rotation";
}

///////////////////////////////////////////////////////////////////////////////
//
// helper that given an angle/axis in world space, returns the matrix for
// the rotation in local space of a node
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// rotate logic
//
///////////////////////////////////////////////////////////////////////////////
function coreRotate(axis) {

    var selectionMode = getSelectionMode();

    var selectedElement = getFirstSelectedWithoutAncestorInSelection();
    if (selectedElement == null) {
        return;
    }

    // get the angle and axis
    var angle = math.getLength(axis);
    axis = math.getNormalized(axis);

    if (selectionMode == 0) {

        //
        // object mode
        //

        // get the selected as node
        var selectedNode = selectedElement.behavior;

        // determine local to world transform
        var localToWorld = selectedElement.getTrait("WorldTransform").value;
        
        // remove scale
        var scale = selectedElement.getTrait("Scale").value;
        var scaleMatrix = math.createScale(scale);
        var scaleInverse = math.getInverse(scaleMatrix);
        localToWorld = math.multiplyMatrix(localToWorld, scaleInverse);

        // get world to local as inverse
        var worldToLocal = math.getInverse(localToWorld);

        // transform the axis into local space
        axis = math.transformNormal(worldToLocal, axis);
        axis = math.getNormalized(axis);

        // compute the rotation matrix in local space
        var rotationDeltaInLocal = math.createRotationAngleAxis(angle, axis);

        // determine the trait name to modify
        var rotationTraitId = getRotationTraitId();

        // get the current rotation value as euler xyz
        var currentRotation = selectedElement.getTrait(rotationTraitId).value;

        // convert to radians
        var factor = 3.14 / 180.0;
        currentRotation[0] *= factor;
        currentRotation[1] *= factor;
        currentRotation[2] *= factor;

        // get the current rotation matrix
        var currentRotationMatrixInLocal = math.createEulerXYZ(
            currentRotation[0],
            currentRotation[1],
            currentRotation[2]
            );

        // compute the new rotation matrix
        var newRotationInLocal = math.multiplyMatrix(currentRotationMatrixInLocal, rotationDeltaInLocal);

        // extract euler angles
        var newXYZ = math.getEulerXYZ(newRotationInLocal);

        // convert to degrees
        var invFactor = 1.0 / factor;
        newXYZ[0] *= invFactor;
        newXYZ[1] *= invFactor;
        newXYZ[2] *= invFactor;

        // check for grid snap
        var isSnapMode = getDesignerPropAsBool("snap");
        if (isSnapMode && stepAmount != 0) {

            //
            // snap to grid is ON
            //

            var targetX = newXYZ[0] + accumDx;
            var targetY = newXYZ[1] + accumDy;
            var targetZ = newXYZ[2] + accumDz;

            var roundedX = Math.round(targetX / stepAmount) * stepAmount;
            var roundedY = Math.round(targetY / stepAmount) * stepAmount;
            var roundedZ = Math.round(targetZ / stepAmount) * stepAmount;

            var halfStep = stepAmount * 0.5;
            var stepPct = halfStep * 0.9;

            var finalXYZ = selectedElement.getTrait(rotationTraitId).value;
            if (Math.abs(roundedX - targetX) < stepPct) {
                finalXYZ[0] = roundedX;
            }

            if (Math.abs(roundedY - targetY) < stepPct) {
                finalXYZ[1] = roundedY;
            }

            if (Math.abs(roundedZ - targetZ) < stepPct) {
                finalXYZ[2] = roundedZ;
            }

            accumDx = targetX - finalXYZ[0];
            accumDy = targetY - finalXYZ[1];
            accumDz = targetZ - finalXYZ[2];

            newXYZ = finalXYZ;
        }

        undoableItem._lastValue = newXYZ;
        undoableItem.onDo();
    }
    else if (selectionMode == 1 || selectionMode == 2 || selectionMode == 3) {
        //
        // polygon or edge selection mode
        //

        localToWorld = selectedElement.getTrait("WorldTransform").value;
        
        // normalize the local to world matrix to remove scale
        var scale = selectedElement.getTrait("Scale").value;
        var scaleMatrix = math.createScale(scale);
        var scaleInverse = math.getInverse(scaleMatrix);
        localToWorld = math.multiplyMatrix(localToWorld, scaleInverse);

        // get world to local as inverse
        var worldToLocal = math.getInverse(localToWorld);

        // transform the axis into local space
        axis = math.transformNormal(worldToLocal, axis);
        axis = math.getNormalized(axis);

        // compute the rotation matrix in local space
        var rotationDeltaInLocal = math.createRotationAngleAxis(angle, axis);
        
        undoableItem._currentDelta = rotationDeltaInLocal;

        undoableItem.onDo();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Listens to manipulator position changes
//
///////////////////////////////////////////////////////////////////////////////
function onManipulatorXYZChangedHandler(sender, args) {
    var axis = manipulatorData.getTrait("RotationManipulatorTraitXYZ").value;
    coreRotate(axis);
}

//
// create an object that can be used to do/undo subobject rotation
//
function UndoableSubobjectRotation(elem) {
    
    this._totalDelta = math.createIdentity();
    this._currentDelta = math.createIdentity();

    // find the mesh child
    this._meshElem = findFirstChildMesh(elem);
    if (this._meshElem == null) {
        return;
    }

    // get the manipulator position. we will use this as our rotation origin
    var rotationOrigin = manipulator.getWorldPosition();

    // get the transform into mesh local space
    var localToWorldMatrix = getWorldMatrix(this._meshElem);
    var worldToLocal = math.getInverse(localToWorldMatrix);

    // transform the manipulator position into mesh space
    this._rotationOrigin = math.transformPoint(worldToLocal, rotationOrigin);

    // get the mesh behavior to use later
    this._mesh = this._meshElem.behavior;

    // get the collection element
    var collElem = this._mesh.selectedObjects;
    if (collElem == null) {
        return;
    }

    // clone the collection element
    this._collectionElement = collElem.clone();

    // get the actual collection we can operate on
    this._collection = this._collectionElement.behavior;
    
    // get the geometry we will operate on
    this._geom = this._meshElem.getTrait("Geometry").value
}

//
// called whenever a manipulation is started
//
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

        var traitId = getRotationTraitId();

        function UndoableRotation(trait, traitValues, initialValue, rotationOffset, urrGeom, restoreGeom, meshes, shouldUsePivot) {
            this._traitArray = traitArray;
            this._traitValues = traitValues;
            this._initialValues = initialValue;
            this._restoreGeom = restoreGeom;
            this._currGeom = currGeom;
            this._rotationOffset = rotationOffset;
            this._meshes = meshes;
            this._shouldUsePivot = shouldUsePivot;
        }

        var traitArray = new Array();
        var traitValues = new Array();
        var initialValues = new Array();
        var restoreGeom = new Array();
        var currGeom = new Array();
        var rotationOffset = new Array();
        var meshes = new Array();

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

                var currTrait = currSelected.getTrait(traitId);

                // get the transform to object space
                var localToWorldMatrix = getWorldMatrix(currSelected);
                var worldToLocal = math.getInverse(localToWorldMatrix);

                // get the manipulators position
                var rotationOriginInWorld = manipulator.getWorldPosition(currSelected);

                // get the manip position in object space
                var rotationPivotInObject = math.transformPoint(worldToLocal, rotationOriginInWorld);

                var meshElem = findFirstChildMesh(currSelected);
                if (meshElem != null) {
                    // save the geometry pointer and a copy of the geometry to restore on undo
                    meshes.push(meshElem.behavior);

                    var geom;
                    geom = meshElem.getTrait("Geometry").value;
                    currGeom.push(geom);
                    restoreGeom.push(geom.clone());
                }
                else {
                    meshes.push(null);
                    currGeom.push(null);
                    restoreGeom.push(null);
                }

                traitArray.push(currTrait);
                traitValues.push(currTrait.value);
                initialValues.push(currTrait.value);
                rotationOffset.push(rotationPivotInObject);
            }
        }

        // create the undoable item
        undoableItem = new UndoableRotation(traitArray, traitValues, initialValues, rotationOffset, currGeom, restoreGeom, meshes, getShouldUsePivot());
        undoableItem.onDo = function () {

            var count = this._traitArray.length;

            // movement delta of all the selected is determined by delta of the first selected
            var delta = [0, 0, 0];
            if (count > 0) {
                delta[0] = this._lastValue[0] - this._initialValues[0][0];
                delta[1] = this._lastValue[1] - this._initialValues[0][1];
                delta[2] = this._lastValue[2] - this._initialValues[0][2];
            }

            var factor = 3.14 / 180.0;
            for (i = 0; i < count; i++) {
                var currTrait = this._traitArray[i];
                this._traitValues[i][0] = this._initialValues[i][0] + delta[0];
                this._traitValues[i][1] = this._initialValues[i][1] + delta[1];
                this._traitValues[i][2] = this._initialValues[i][2] + delta[2];

                var theVal = [0, 0, 0];
                theVal[0] = this._traitValues[i][0];
                theVal[1] = this._traitValues[i][1];
                theVal[2] = this._traitValues[i][2];

                // get the current rotation matrix
                if (this._shouldUsePivot) {
                    var theOldVal = this._traitArray[i].value;
                    var currentRotationMatrixInLocal = math.createEulerXYZ(factor * theOldVal[0], factor * theOldVal[1], factor * theOldVal[2]);

                    // get the new rotation matrix
                    var newRotationInLocal = math.createEulerXYZ(factor * theVal[0], factor * theVal[1], factor * theVal[2]);

                    // get the inverse rotation of the old rotation
                    var toOldRotation = math.getInverse(currentRotationMatrixInLocal);

                    // compute the rotation delta as matrix
                    var rotationDelta = math.multiplyMatrix(toOldRotation, newRotationInLocal);
                    rotationDelta = math.getInverse(rotationDelta);

                    // we want to rotate relative to the manipulator position
                    if (this._meshes[i] != null) {
                        var translationMatrix = math.createTranslation(this._rotationOffset[i][0], this._rotationOffset[i][1], this._rotationOffset[i][2]);
                        var invTranslationMatrix = math.getInverse(translationMatrix);
                        var transform = math.multiplyMatrix(rotationDelta, invTranslationMatrix);
                        transform = math.multiplyMatrix(translationMatrix, transform);

                        // apply inverse delta to geometry
                        this._currGeom[i].transform(transform);
                        this._meshes[i].recomputeCachedGeometry();
                    }
                }

                // set the rotation trait value
                this._traitArray[i].value = theVal;
            }
        }

        undoableItem.onUndo = function () {
            var count = this._traitArray.length;
            for (i = 0; i < count; i++) {
                this._traitArray[i].value = this._initialValues[i];
                if (this._shouldUsePivot) {
                    if (this._meshes[i] != null) {
                        this._currGeom[i].copyFrom(this._restoreGeom[i]);
                        this._meshes[i].recomputeCachedGeometry();
                    }
                }
            }
        }
    }
    else {

        // create the undoable item
        undoableItem = new UndoableSubobjectRotation(document.selectedElement);

        if (selectionMode == 1) {
            // polygon selection mode
            undoableItem.getPoints = function () {

                // use map/hash in object to eliminate dups in collection 
                var points = new Object();

                // loop over the point indices in the poly collection
                var polyCount = this._collection.getPolygonCount();
                for (var i = 0; i < polyCount; i++) {
                    var polyIndex = this._collection.getPolygon(i);

                    // get the point count and loop over polygon points
                    var polygonPointCount = this._geom.getPolygonPointCount(polyIndex);
                    for (var j = 0; j < polygonPointCount; j++) {

                        // get the point index
                        var pointIndex = this._geom.getPolygonPoint(polyIndex, j);
                        points[pointIndex] = pointIndex;
                    }
                }
                return points;
            }
        }
        else if (selectionMode == 2) {
            // edge selection mode
            undoableItem.getPoints = function () {

                // use map/hash in object to eliminate dups in collection 
                var points = new Object();

                // loop over the edges
                var edgeCount = this._collection.getEdgeCount();
                for (var i = 0; i < edgeCount; i++) {
                    var edge = this._collection.getEdge(i);

                    points[edge[0]] = edge[0];
                    points[edge[1]] = edge[1];
                }
                return points;
            }
        }
        else if (selectionMode == 3) {
            // edge selection mode
            undoableItem.getPoints = function () {

                // use map/hash in object to eliminate dups in collection
                var points = new Object();

                // loop over the points
                var ptCount = this._collection.getPointCount();
                for (var i = 0; i < ptCount; i++) {
                    var pt = this._collection.getPoint(i);

                    points[pt] = pt;
                }
                return points;
            }
        }

        // do
        undoableItem.onDo = function () {

            // we want to rotate relative to the manipulator position
            var polygonPoints = this.getPoints()

            var translationMatrix = math.createTranslation(this._rotationOrigin[0], this._rotationOrigin[1], this._rotationOrigin[2]);
            var invTranslationMatrix = math.getInverse(translationMatrix);
            var transform = math.multiplyMatrix(this._currentDelta, invTranslationMatrix);
            transform = math.multiplyMatrix(translationMatrix, transform);

            // loop over the unique set of indices and transform the associated point
            for (var key in polygonPoints) {
                var ptIdx = polygonPoints[key];
                var pt = this._geom.getPointAt(ptIdx);

                pt = math.transformPoint(transform, pt);

                this._geom.setPointAt(ptIdx, pt);
            }

            this._totalDelta = math.multiplyMatrix(this._currentDelta, this._totalDelta);

            // invalidate the mesh collision
            this._mesh.recomputeCachedGeometry();
        }

        //
        // undo
        //
        undoableItem.onUndo = function () {

            // we want to rotate relative to the manipulator position
            var polygonPoints = this.getPoints()

            var invTotal = math.getInverse(this._totalDelta);

            // we want to rotate relative to the manipulator position
            var translationMatrix = math.createTranslation(this._rotationOrigin[0], this._rotationOrigin[1], this._rotationOrigin[2]);
            var invTranslationMatrix = math.getInverse(translationMatrix);
            var transform = math.multiplyMatrix(invTotal, invTranslationMatrix);
            transform = math.multiplyMatrix(translationMatrix, transform);

            // loop over the unique set of indices and transform the associated point
            for (var key in polygonPoints) {
                var ptIdx = polygonPoints[key];
                var pt = this._geom.getPointAt(ptIdx);

                pt = math.transformPoint(transform, pt);

                this._geom.setPointAt(ptIdx, pt);
            }

            this._currentDelta = this._totalDelta;
            this._totalDelta = math.createIdentity();

            this._mesh.recomputeCachedGeometry();
        }
    }

    if (undoableItem != null) {
        //
        // getName()
        //
        undoableItem.getName = function () {
            var IDS_MreUndoRotate = 144;
            return services.strings.getStringFromId(IDS_MreUndoRotate);
        }

        services.undoService.addUndoableItem(undoableItem);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// tool method implementations
//
///////////////////////////////////////////////////////////////////////////////

tool.activate = function () {
    state.value = 2;

    createOptions();

    services.manipulators.activate("RotationManipulator");

    onBeginManipulationHandler = manipulator.addHandler("OnBeginManipulation", onBeginManipulation);

    manipulatorTraitXYZTraitChangedCookie = mxyz.addHandler("OnDataChanged", onManipulatorXYZChangedHandler);
}

tool.deactivate = function () {
    state.value = 0;

    toolProps.getTrait("StepAmount").removeHandler("OnDataChanged", toolPropCookie);

    var snapTrait = document.designerProps.getOrCreateTrait("snap", "bool", 0);
    snapTrait.removeHandler("OnDataChanged", snapCookie);

    mxyz.removeHandler("OnDataChanged", manipulatorTraitXYZTraitChangedCookie);
    services.manipulators.deactivate("RotationManipulator");

    manipulator.removeHandler("OnBeginManipulation", onBeginManipulationHandler);
}

// If we're already running, do nothing
if (state.value != 2) {
    document.setTool(tool);
}

// SIG // Begin signature block
// SIG // MIIjnQYJKoZIhvcNAQcCoIIjjjCCI4oCAQExDzANBglg
// SIG // hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
// SIG // BgEEAYI3AgEeMCQCAQEEEBDgyQbOONQRoqMAEEvTUJAC
// SIG // AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
// SIG // ExknoqtH+sUrBFmA6SevC9vPXY7sKeLpyqrvkPzsOWCg
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
// SIG // SEXAQsmbdlsKgEhr/Xmfwb1tbWrJUnMTDXpQzTGCFXQw
// SIG // ghVwAgEBMIGVMH4xCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKDAm
// SIG // BgNVBAMTH01pY3Jvc29mdCBDb2RlIFNpZ25pbmcgUENB
// SIG // IDIwMTECEzMAAAHfa/AukqdKtNAAAAAAAd8wDQYJYIZI
// SIG // AWUDBAIBBQCgga4wGQYJKoZIhvcNAQkDMQwGCisGAQQB
// SIG // gjcCAQQwHAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcC
// SIG // ARUwLwYJKoZIhvcNAQkEMSIEIL2leogBpbFTl1r5Nkoz
// SIG // c1HzFuhMK0uhCPhZHaS+C4IyMEIGCisGAQQBgjcCAQwx
// SIG // NDAyoBSAEgBNAGkAYwByAG8AcwBvAGYAdKEagBhodHRw
// SIG // Oi8vd3d3Lm1pY3Jvc29mdC5jb20wDQYJKoZIhvcNAQEB
// SIG // BQAEggEApYSaZYtPVCpFqLCDMVKjwnK/Uu1YebmBFnSg
// SIG // 4UwiMrc4RsPVNPaknQ9CDG0zdxNmyJaeLK8e4esX15W2
// SIG // Txiil0uCuObunUWHC6s4DkDaVGCPxdFMM+Ec4InBXdkD
// SIG // dB8VwHRs1F4cqiR62sdQkZ+N7tN7vtqg2xpcOEZvH3X3
// SIG // LW1lPgJ193Qdovm3XTNQf8FqXqwN/86Om0YtGXI0F4hW
// SIG // 8maYNnvaMyL1Z0sdACQqjkeNryqAZJQS/cgQ596+k9/b
// SIG // IUCa3sB365qnQnAjo1dJPx+baPh5tR96MwPjnkFbmGUD
// SIG // zjtWth+QKi4/AUHCOav0B7QKdYh9xTDNdPgMP6HPLqGC
// SIG // Ev4wghL6BgorBgEEAYI3AwMBMYIS6jCCEuYGCSqGSIb3
// SIG // DQEHAqCCEtcwghLTAgEDMQ8wDQYJYIZIAWUDBAIBBQAw
// SIG // ggFZBgsqhkiG9w0BCRABBKCCAUgEggFEMIIBQAIBAQYK
// SIG // KwYBBAGEWQoDATAxMA0GCWCGSAFlAwQCAQUABCC9UrUU
// SIG // sGUvQLp550rU/Wc2BznPKjqgzdeGDwbBgdkEwAIGYLDq
// SIG // pnA2GBMyMDIxMDYxMTAwMjUzMi4xNzRaMASAAgH0oIHY
// SIG // pIHVMIHSMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMS0wKwYDVQQL
// SIG // EyRNaWNyb3NvZnQgSXJlbGFuZCBPcGVyYXRpb25zIExp
// SIG // bWl0ZWQxJjAkBgNVBAsTHVRoYWxlcyBUU1MgRVNOOjhE
// SIG // NDEtNEJGNy1CM0I3MSUwIwYDVQQDExxNaWNyb3NvZnQg
// SIG // VGltZS1TdGFtcCBTZXJ2aWNloIIOTTCCBPkwggPhoAMC
// SIG // AQICEzMAAAE6jY0x93dJScIAAAAAATowDQYJKoZIhvcN
// SIG // AQELBQAwfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldh
// SIG // c2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNV
// SIG // BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UE
// SIG // AxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAw
// SIG // HhcNMjAxMDE1MTcyODIyWhcNMjIwMTEyMTcyODIyWjCB
// SIG // 0jELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
// SIG // b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
// SIG // Y3Jvc29mdCBDb3Jwb3JhdGlvbjEtMCsGA1UECxMkTWlj
// SIG // cm9zb2Z0IElyZWxhbmQgT3BlcmF0aW9ucyBMaW1pdGVk
// SIG // MSYwJAYDVQQLEx1UaGFsZXMgVFNTIEVTTjo4RDQxLTRC
// SIG // RjctQjNCNzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUt
// SIG // U3RhbXAgU2VydmljZTCCASIwDQYJKoZIhvcNAQEBBQAD
// SIG // ggEPADCCAQoCggEBAM5fJOdfD5c/CUyF2J/zvTmpKnFq
// SIG // nSfGVyYQJLPYciwfPgDVu2Z4G9be4sO05oHqKDsDJ24Q
// SIG // kEB9eFMFnfIUBVnsSqlaXCBZ2N29efj2JoFXkOFyn1id
// SIG // zzzI04l7u93qVx4/V1+5oUtiFasBLwzrnTiN8kC/A/Dj
// SIG // htuG/tdMwwxMjecL2eQNVnL5dnkIQhutRnhzWl71zpaE
// SIG // 0G5VRWzFjphr4h74EZUYUY4DZrr9Sdsun0BbYhMNKXVy
// SIG // gkBuvMUJQYXLuC5/m2C+B6Hk9pq7ZKRxMg2fn66/imv8
// SIG // 1Az9wbHB5CnZDRjjg37yXVh2ldFs69cJz7lILTm35wTt
// SIG // KEoyKQUCAwEAAaOCARswggEXMB0GA1UdDgQWBBQUVqCf
// SIG // Fl8Sa6bJbxx1rK0JhUXzyzAfBgNVHSMEGDAWgBTVYzpc
// SIG // ijGQ80N7fEYbxTNoWoVtVTBWBgNVHR8ETzBNMEugSaBH
// SIG // hkVodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtpL2Ny
// SIG // bC9wcm9kdWN0cy9NaWNUaW1TdGFQQ0FfMjAxMC0wNy0w
// SIG // MS5jcmwwWgYIKwYBBQUHAQEETjBMMEoGCCsGAQUFBzAC
// SIG // hj5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpL2Nl
// SIG // cnRzL01pY1RpbVN0YVBDQV8yMDEwLTA3LTAxLmNydDAM
// SIG // BgNVHRMBAf8EAjAAMBMGA1UdJQQMMAoGCCsGAQUFBwMI
// SIG // MA0GCSqGSIb3DQEBCwUAA4IBAQBeN+Q+pAEto3gCfARt
// SIG // Sk5uQM9I6W3Y6akrzC7e3zla2gBB5XYJNOASnE5oc420
// SIG // 17I8IAjDAkvo1+E6KotHJ83EqA9i6YWQPfA13h+pUR+6
// SIG // kHV2x4MdP726tGMlZJbUkzL9Y88yO+WeXYab3OFMJ+BW
// SIG // +ggmhdv/q+0a9I/tgxgsJfGlqr0Ks4Qif6O3DaGFHyeF
// SIG // kgh7NTrI4DJlk5oJbjyMbd5FfIi2ZdiGYMq+XDTojYk6
// SIG // 2hQ6u4cSNeqA7KFtBECglfOJ1dcFklntUogCiMK+Qbam
// SIG // MCfoZaHD8OanoVmwYky57XC68CnQG8LYRa9Qx390WMBa
// SIG // A88jbSkgf0ybAdmzMIIGcTCCBFmgAwIBAgIKYQmBKgAA
// SIG // AAAAAjANBgkqhkiG9w0BAQsFADCBiDELMAkGA1UEBhMC
// SIG // VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
// SIG // B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
// SIG // b3JhdGlvbjEyMDAGA1UEAxMpTWljcm9zb2Z0IFJvb3Qg
// SIG // Q2VydGlmaWNhdGUgQXV0aG9yaXR5IDIwMTAwHhcNMTAw
// SIG // NzAxMjEzNjU1WhcNMjUwNzAxMjE0NjU1WjB8MQswCQYD
// SIG // VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
// SIG // A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
// SIG // IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQg
// SIG // VGltZS1TdGFtcCBQQ0EgMjAxMDCCASIwDQYJKoZIhvcN
// SIG // AQEBBQADggEPADCCAQoCggEBAKkdDbx3EYo6IOz8E5f1
// SIG // +n9plGt0VBDVpQoAgoX77XxoSyxfxcPlYcJ2tz5mK1vw
// SIG // FVMnBDEfQRsalR3OCROOfGEwWbEwRA/xYIiEVEMM1024
// SIG // OAizQt2TrNZzMFcmgqNFDdDq9UeBzb8kYDJYYEbyWEeG
// SIG // MoQedGFnkV+BVLHPk0ySwcSmXdFhE24oxhr5hoC732H8
// SIG // RsEnHSRnEnIaIYqvS2SJUGKxXf13Hz3wV3WsvYpCTUBR
// SIG // 0Q+cBj5nf/VmwAOWRH7v0Ev9buWayrGo8noqCjHw2k4G
// SIG // kbaICDXoeByw6ZnNPOcvRLqn9NxkvaQBwSAJk3jN/LzA
// SIG // yURdXhacAQVPIk0CAwEAAaOCAeYwggHiMBAGCSsGAQQB
// SIG // gjcVAQQDAgEAMB0GA1UdDgQWBBTVYzpcijGQ80N7fEYb
// SIG // xTNoWoVtVTAZBgkrBgEEAYI3FAIEDB4KAFMAdQBiAEMA
// SIG // QTALBgNVHQ8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAf
// SIG // BgNVHSMEGDAWgBTV9lbLj+iiXGJo0T2UkFvXzpoYxDBW
// SIG // BgNVHR8ETzBNMEugSaBHhkVodHRwOi8vY3JsLm1pY3Jv
// SIG // c29mdC5jb20vcGtpL2NybC9wcm9kdWN0cy9NaWNSb29D
// SIG // ZXJBdXRfMjAxMC0wNi0yMy5jcmwwWgYIKwYBBQUHAQEE
// SIG // TjBMMEoGCCsGAQUFBzAChj5odHRwOi8vd3d3Lm1pY3Jv
// SIG // c29mdC5jb20vcGtpL2NlcnRzL01pY1Jvb0NlckF1dF8y
// SIG // MDEwLTA2LTIzLmNydDCBoAYDVR0gAQH/BIGVMIGSMIGP
// SIG // BgkrBgEEAYI3LgMwgYEwPQYIKwYBBQUHAgEWMWh0dHA6
// SIG // Ly93d3cubWljcm9zb2Z0LmNvbS9QS0kvZG9jcy9DUFMv
// SIG // ZGVmYXVsdC5odG0wQAYIKwYBBQUHAgIwNB4yIB0ATABl
// SIG // AGcAYQBsAF8AUABvAGwAaQBjAHkAXwBTAHQAYQB0AGUA
// SIG // bQBlAG4AdAAuIB0wDQYJKoZIhvcNAQELBQADggIBAAfm
// SIG // iFEN4sbgmD+BcQM9naOhIW+z66bM9TG+zwXiqf76V20Z
// SIG // MLPCxWbJat/15/B4vceoniXj+bzta1RXCCtRgkQS+7lT
// SIG // jMz0YBKKdsxAQEGb3FwX/1z5Xhc1mCRWS3TvQhDIr79/
// SIG // xn/yN31aPxzymXlKkVIArzgPF/UveYFl2am1a+THzvbK
// SIG // egBvSzBEJCI8z+0DpZaPWSm8tv0E4XCfMkon/VWvL/62
// SIG // 5Y4zu2JfmttXQOnxzplmkIz/amJ/3cVKC5Em4jnsGUpx
// SIG // Y517IW3DnKOiPPp/fZZqkHimbdLhnPkd/DjYlPTGpQqW
// SIG // hqS9nhquBEKDuLWAmyI4ILUl5WTs9/S/fmNZJQ96LjlX
// SIG // dqJxqgaKD4kWumGnEcua2A5HmoDF0M2n0O99g/DhO3EJ
// SIG // 3110mCIIYdqwUB5vvfHhAN/nMQekkzr3ZUd46PioSKv3
// SIG // 3nJ+YWtvd6mBy6cJrDm77MbL2IK0cs0d9LiFAR6A+xuJ
// SIG // KlQ5slvayA1VmXqHczsI5pgt6o3gMy4SKfXAL1QnIffI
// SIG // rE7aKLixqduWsqdCosnPGUFN4Ib5KpqjEWYw07t0Mkvf
// SIG // Y3v1mYovG8chr1m1rtxEPJdQcdeh0sVV42neV8HR3jDA
// SIG // /czmTfsNv11P6Z0eGTgvvM9YBS7vDaBQNdrvCScc1bN+
// SIG // NR4Iuto229Nfj950iEkSoYIC1zCCAkACAQEwggEAoYHY
// SIG // pIHVMIHSMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMS0wKwYDVQQL
// SIG // EyRNaWNyb3NvZnQgSXJlbGFuZCBPcGVyYXRpb25zIExp
// SIG // bWl0ZWQxJjAkBgNVBAsTHVRoYWxlcyBUU1MgRVNOOjhE
// SIG // NDEtNEJGNy1CM0I3MSUwIwYDVQQDExxNaWNyb3NvZnQg
// SIG // VGltZS1TdGFtcCBTZXJ2aWNloiMKAQEwBwYFKw4DAhoD
// SIG // FQAHJZHZ9Y9YF4Hcr2I+NQfK5DWeCKCBgzCBgKR+MHwx
// SIG // CzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9u
// SIG // MRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNy
// SIG // b3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jv
// SIG // c29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMA0GCSqGSIb3
// SIG // DQEBBQUAAgUA5GyL+zAiGA8yMDIxMDYxMDIxMDI1MVoY
// SIG // DzIwMjEwNjExMjEwMjUxWjB3MD0GCisGAQQBhFkKBAEx
// SIG // LzAtMAoCBQDkbIv7AgEAMAoCAQACAhV5AgH/MAcCAQAC
// SIG // AhGUMAoCBQDkbd17AgEAMDYGCisGAQQBhFkKBAIxKDAm
// SIG // MAwGCisGAQQBhFkKAwKgCjAIAgEAAgMHoSChCjAIAgEA
// SIG // AgMBhqAwDQYJKoZIhvcNAQEFBQADgYEAt9lXWICv0iAb
// SIG // 5EcQyYRiHpyP4/KTLiM4q/N3kDBKtBPNfGA35fSd8Fdl
// SIG // y5Xb3bz6L29MGDJmg3PpY9sl9zPXKJxP/LrZ2r8tJqpb
// SIG // ECu52GW2MppdNvhXqRdrdH7zRorxKq+Gtco89SpfhSvJ
// SIG // ErnSBNSdV7SEpqCfG9izbrLamv0xggMNMIIDCQIBATCB
// SIG // kzB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGlu
// SIG // Z3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMV
// SIG // TWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1N
// SIG // aWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMAITMwAA
// SIG // ATqNjTH3d0lJwgAAAAABOjANBglghkgBZQMEAgEFAKCC
// SIG // AUowGgYJKoZIhvcNAQkDMQ0GCyqGSIb3DQEJEAEEMC8G
// SIG // CSqGSIb3DQEJBDEiBCB07iOy8Io1gfS7kTrxh3mB1/+b
// SIG // BTidofKOCFIag0a5ZDCB+gYLKoZIhvcNAQkQAi8xgeow
// SIG // gecwgeQwgb0EIJ+v0IQHqSxf+wXbL37vBjk/ooS/XOHI
// SIG // KOTX9WlDfLRtMIGYMIGApH4wfDELMAkGA1UEBhMCVVMx
// SIG // EzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1Jl
// SIG // ZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3Jh
// SIG // dGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3Rh
// SIG // bXAgUENBIDIwMTACEzMAAAE6jY0x93dJScIAAAAAATow
// SIG // IgQg6/p/ik9bcaodO3vnZbJZmlgB9o23px12h0AeHMAa
// SIG // MpYwDQYJKoZIhvcNAQELBQAEggEApq/5AqevFPq18dFn
// SIG // /1GlLXgseFcVm7o5HP6uWDjbLIkZsk4ENMCdy8TW4U1f
// SIG // tp4TpP07GcpFkUC97tAbZ3WGdt/Kp+7z1qM8fEvLj+j+
// SIG // hZ24JUmeGLRPPzmlxAf1jdRdcSn3O+dsGIF//44sGURM
// SIG // R05s5jbAudTwNGg2ah+oXg3KnlpMmcuMvtv79+5I9rEV
// SIG // szmfAq3V7nm53/ATMPpocQ2UJX5VeIvm4Hj7ywmj+gPK
// SIG // a/P98qzlADAHg3vIFzRYEBIYwWuB4b41sCYwndxdz3iP
// SIG // CgOZbs/S3EDRWtRbEulMtKIw1IXyShpZVY5uYaeYhPId
// SIG // bKZgfgvraonMOAWQcw==
// SIG // End signature block
