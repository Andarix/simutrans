
//
// AddSubdivisions.js
//
function addSubdivisions(geom, polyCollection, polyIndex, splitEdgesMap, polysToSelect) {

    var polygonPointCount = geom.getPolygonPointCount(polyIndex);
    if (polygonPointCount < 3) {
        return;
    }
    // services.debug.trace("splitting poly : " + polyIndex);

    var containingMesh = polyCollection.getContainingMesh();

    // determine if we need to add new texture coordinates
    var IndexingModeUndefined = 0;
    var IndexingModePerPoint = 1;
    var IndexingModePerPointOnPolygon = 3;

    // determine the material index on the pre division polygon
    var materialIndex = geom.getPolygonMaterialIndex(polyIndex);

    // this is our starting index for new points
    var newPointStart = geom.pointCount;

    // we'll compute the center of the triangle
    var avgPos = [0, 0, 0];
    var avgTex = [0, 0];

    // iterate over polypoints and total
    for (var i = 0; i < polygonPointCount; i++) {
        var point = geom.getPointOnPolygon(polyIndex, i);
        avgPos[0] += point[0];
        avgPos[1] += point[1];
        avgPos[2] += point[2];
    }

    // total tex coords if needed
    if (geom.textureCoordinateIndexingMode != IndexingModeUndefined) {
        for (var i = 0; i < polygonPointCount; i++) {
            
            var texCoord = geom.getTextureCoordinateOnPolygon(polyIndex, i);
            avgTex[0] += texCoord[0];
            avgTex[1] += texCoord[1];
        }
    }

    // get the center 
    var div = 1.0 / polygonPointCount;
    avgPos[0] *= div;
    avgPos[1] *= div;
    avgPos[2] *= div;

    avgTex[0] *= div;
    avgTex[1] *= div;

    // add the avg point
    geom.addPoints(avgPos, 1);

    // store tex coords if (if we need to)
    // we'll put these into the geom later
    var texcoords = new Array();
    if (geom.textureCoordinateIndexingMode == IndexingModePerPoint) {
        geom.addTextureCoordinates(avgTex, 1);
    }
    else if (geom.textureCoordinateIndexingMode == IndexingModePerPointOnPolygon) {
        texcoords.push(avgTex);
    }

    // split points for this polygon
    var splitPointIndices = new Array();
    
    // compute the split points
    for (var i = 0; i < polygonPointCount; i++) {
        var i0 = i;
        var i1 = i + 1;
        if (i1 >= polygonPointCount) {
            i1 = 0;
        }
        var p0 = geom.getPointOnPolygon(polyIndex, i0);
        var p1 = geom.getPointOnPolygon(polyIndex, i1);

        // get object level indices
        var oi0 = geom.getPolygonPoint(polyIndex, i0);
        var oi1 = geom.getPolygonPoint(polyIndex, i1);

        // index of the point that splits this edge
        var splitPointIndex = 0;

        // we're splitting the edge between object level points
        // p0 and p1, so see if this is edge has been previously split

        var tmp0 = Math.min(oi0, oi1);
        var tmp1 = Math.max(oi0, oi1);

        // services.debug.trace("splitting edge: " + tmp0 + " " + tmp1);

        var wasNewPointCreated = false;
        if (splitEdgesMap.hasOwnProperty("" + tmp0) && (splitEdgesMap[tmp0].hasOwnProperty("" + tmp1)))
        {
            splitPointIndex = splitEdgesMap[tmp0][tmp1];
            // services.debug.trace("using split point: " + splitPointIndex);
        }
        else
        {
            wasNewPointCreated = true;

            var split = [0, 0, 0];
            split[0] = p0[0] + 0.5 * (p1[0] - p0[0]);
            split[1] = p0[1] + 0.5 * (p1[1] - p0[1]);
            split[2] = p0[2] + 0.5 * (p1[2] - p0[2]);

            if (splitEdgesMap.hasOwnProperty("" + tmp0) == false) {
                splitEdgesMap[tmp0] = new Object();
            }

            splitPointIndex = geom.pointCount;
            geom.addPoints(split, 1);

            // services.debug.trace("created split point: " + splitPointIndex);

            splitEdgesMap[tmp0][tmp1] = splitPointIndex;
        }
        splitPointIndices.push(splitPointIndex);

        if (geom.textureCoordinateIndexingMode != IndexingModeUndefined) {

            var texCoord0 = geom.getTextureCoordinateOnPolygon(polyIndex, i0);
            var texCoord1 = geom.getTextureCoordinateOnPolygon(polyIndex, i1);
            var texCoord = [0, 0];
            texCoord[0] = texCoord0[0] + 0.5 * (texCoord1[0] - texCoord0[0]);
            texCoord[1] = texCoord0[1] + 0.5 * (texCoord1[1] - texCoord0[1]);

            if (geom.textureCoordinateIndexingMode == IndexingModePerPoint) {
                if (wasNewPointCreated) {
                    geom.addTextureCoordinates(texCoord, 1);
                }   
            }
            else if (geom.textureCoordinateIndexingMode == IndexingModePerPointOnPolygon) {
                texcoords.push(texCoord);
            }
        }

        //
        // we need to split edges on polys if the edge is shared
        //
        var polygonsSharingEdge = geom.getPolygonsFromEdge(tmp0, tmp1);
        var sharedPolyCount = polygonsSharingEdge.getPolygonCount();
        for (var j = 0; j < sharedPolyCount; j++) {
            var sharedPolygonIndex = polygonsSharingEdge.getPolygon(j);
            if (polyCollection.hasPolygon(sharedPolygonIndex) == false) {
                // this is a shared poly, and not part of the set we're going to subdivide explicitly
                // so we need to break the edge tmp0, tmp1
                geom.insertPolygonPoint(sharedPolygonIndex, tmp0, tmp1, splitPointIndex);
            }
        }
    }

    // we now create a new polygon (quad) for each point in the old poly
    // i.e oldpoly point count == old poly edge count == number of new polys created after split
    for (var i = 0; i < polygonPointCount; i++) {

        // wrap around
        var prevIndex;
        if (i > 0) {
            prevIndex = i - 1;
        }
        else {
            prevIndex = polygonPointCount - 1;
        }

        // add a polygon
        var thisPolyIndex = geom.polygonCount;
        geom.addPolygon(materialIndex);

        polysToSelect.push(thisPolyIndex);

        // add points
        var i0 = geom.getPolygonPoint(polyIndex, i);
        var i1 = splitPointIndices[i];
        var i2 = newPointStart;
        var i3 = splitPointIndices[prevIndex];

        geom.addPolygonPoint(thisPolyIndex, i0);
        geom.addPolygonPoint(thisPolyIndex, i1);
        geom.addPolygonPoint(thisPolyIndex, i2);
        geom.addPolygonPoint(thisPolyIndex, i3);

        if (geom.textureCoordinateIndexingMode == IndexingModePerPointOnPolygon) {
            geom.addTextureCoordinates(geom.getTextureCoordinateOnPolygon(polyIndex, i), 1);
            geom.addTextureCoordinates(texcoords[i + 1], 1);
            geom.addTextureCoordinates(texcoords[0], 1);
            geom.addTextureCoordinates(texcoords[prevIndex + 1], 1);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// helper to get a designer property as a bool
//
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


// find the mesh child
function findFirstChildMeshElement(parent)
{
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


function UndoableItem(collElem, meshElem) {

    this._clonedColl = collElem.clone();
    this._polyCollection = this._clonedColl.behavior;
    this._meshElem = meshElem;
    this._mesh = meshElem.behavior;

    var geom = this._meshElem.getTrait("Geometry").value;
    this._restoreGeom = geom.clone();

    this.getName = function () {
        var IDS_MreUndoSubdivide = 147;
        return services.strings.getStringFromId(IDS_MreUndoSubdivide);
    }

    this.onDo = function () {

        var newCollection = this._clonedColl.clone();
        var newPolyBeh = newCollection.behavior;
        newPolyBeh.clear();

        // maps split edges to indices of points used to split them
        var splitEdgesMap = new Object();

        var geom = this._meshElem.getTrait("Geometry").value;

        var polysToSelect = new Array();

        // subdivide
        var polysToDelete = new Array();
        var polyCount = this._polyCollection.getPolygonCount();
        for (var i = 0; i < polyCount; i++) {
            var polyIndex = this._polyCollection.getPolygon(i);
            addSubdivisions(geom, this._polyCollection, polyIndex, splitEdgesMap, polysToSelect);

            polysToDelete.push(polyIndex);
        }

        function sortNumberDescending(a, b) {
            return b - a;
        }
 
        // delete the old selection
        polysToDelete.sort(sortNumberDescending);

        var numDeletedPolys = polysToDelete.length;

        for (var p = 0; p < polysToDelete.length; p++) {
            geom.removePolygon(polysToDelete[p]);
        }

        // shift polygon indices
        for (var p = 0; p < polysToSelect.length; p++) {
            var indexToSelect = polysToSelect[p] - numDeletedPolys;

            newPolyBeh.addPolygon(indexToSelect);
        }

        this._mesh.selectedObjects = newCollection;

        this._mesh.recomputeCachedGeometry();
    }

    this.onUndo = function () {
        var geom = this._meshElem.getTrait("Geometry").value;
        geom.copyFrom(this._restoreGeom);

        this._mesh.selectedObjects = this._clonedColl;

        this._mesh.recomputeCachedGeometry();
    }
}

var selectedElement = document.selectedElement;
var selectionMode = getSelectionMode();

// get the poly collection
var polyCollection = null;
var mesh = null;
var meshElem = null;
var collElem = null;
if (selectedElement != null) {
    if (selectionMode == 1) {
        meshElem = findFirstChildMeshElement(selectedElement);
        if (meshElem != null) {
            mesh = meshElem.behavior;

            // polygon selection mode
            collElem = mesh.selectedObjects;
            if (collElem != null) {
                polyCollection = collElem.behavior;
            }
        }
    }
}

if (polyCollection != null && collElem.typeId == "PolygonCollection") {
    var undoableItem = new UndoableItem(collElem, meshElem);
    undoableItem.onDo();
    services.undoService.addUndoableItem(undoableItem);
}
// SIG // Begin signature block
// SIG // MIIjhAYJKoZIhvcNAQcCoIIjdTCCI3ECAQExDzANBglg
// SIG // hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
// SIG // BgEEAYI3AgEeMCQCAQEEEBDgyQbOONQRoqMAEEvTUJAC
// SIG // AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
// SIG // vVb+u/YO+8d4nxqG8U1pDSpZo+x+WxGf5RzP6XhH35eg
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
// SIG // ARUwLwYJKoZIhvcNAQkEMSIEIIAGLOI8qMuSAI9Msp8J
// SIG // MbWb90tOHn+Qm9EmhZujX6nHMEIGCisGAQQBgjcCAQwx
// SIG // NDAyoBSAEgBNAGkAYwByAG8AcwBvAGYAdKEagBhodHRw
// SIG // Oi8vd3d3Lm1pY3Jvc29mdC5jb20wDQYJKoZIhvcNAQEB
// SIG // BQAEggEAbemRy34pleRs5xRkirJcXFjSWJnKflIxreR3
// SIG // aF944skjt26FXF4bpMRQbIJOqZMVfbzSJl+ytOnvhQnG
// SIG // v0EdU9cQ90/jJoOpOH6pzPBOtl4PitdCp9+3Orz0hJ7k
// SIG // +hehF6YbYQp5aXm6DRRhfpys0zHjV81tOH0rLrNuCxW9
// SIG // 1bNMDdJb7FzW7jggqRQl2tpyA+R8JdVujwmPLshwxgF7
// SIG // wzOuaRpxuSdyRWKzzcwIXKPECjDgKAHmq9MomY8ivDFa
// SIG // wt9Nu9rHnmZ00QC5EakDyTYSEUDRp8lxkxIuRfrObkK2
// SIG // h/cqbc0bwk601Oi++dMvVKo3NROz/+USr/YBRoyL86GC
// SIG // EuUwghLhBgorBgEEAYI3AwMBMYIS0TCCEs0GCSqGSIb3
// SIG // DQEHAqCCEr4wghK6AgEDMQ8wDQYJYIZIAWUDBAIBBQAw
// SIG // ggFRBgsqhkiG9w0BCRABBKCCAUAEggE8MIIBOAIBAQYK
// SIG // KwYBBAGEWQoDATAxMA0GCWCGSAFlAwQCAQUABCBrksv9
// SIG // YN2nkmxYXYFPeL9A7orDzw6wRd1U/ZDulLmG9gIGYK5Z
// SIG // XPAOGBMyMDIxMDYxMTAwMjUyNy42MjRaMASAAgH0oIHQ
// SIG // pIHNMIHKMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSUwIwYDVQQL
// SIG // ExxNaWNyb3NvZnQgQW1lcmljYSBPcGVyYXRpb25zMSYw
// SIG // JAYDVQQLEx1UaGFsZXMgVFNTIEVTTjpBRTJDLUUzMkIt
// SIG // MUFGQzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUtU3Rh
// SIG // bXAgU2VydmljZaCCDjwwggTxMIID2aADAgECAhMzAAAB
// SIG // SKKIRVa8L4C/AAAAAAFIMA0GCSqGSIb3DQEBCwUAMHwx
// SIG // CzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9u
// SIG // MRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNy
// SIG // b3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jv
// SIG // c29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMB4XDTIwMTEx
// SIG // MjE4MjU1NloXDTIyMDIxMTE4MjU1NlowgcoxCzAJBgNV
// SIG // BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
// SIG // VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
// SIG // Q29ycG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBB
// SIG // bWVyaWNhIE9wZXJhdGlvbnMxJjAkBgNVBAsTHVRoYWxl
// SIG // cyBUU1MgRVNOOkFFMkMtRTMyQi0xQUZDMSUwIwYDVQQD
// SIG // ExxNaWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNlMIIB
// SIG // IjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA9/94
// SIG // rxWEitHRrXF2mfKTSxAEW62iXsK8ovyYG2ipau4YRSFN
// SIG // RJGNBzIhUZtv1rLpLetpNuc7w8Vten6YYOfO+vpdcGJw
// SIG // pYHv6xZa0dlLtVsLRZnNcRqcuPrsaGwhDHiBQxoS+QzU
// SIG // 22zorC6wVGG8l6+3z88W1ZBMCQz+RRtp9K6Najo3oCJC
// SIG // yI11OqmoXbkMU37DzhKfI4KjCp/vn6R+n71ypXsFs3bH
// SIG // 74YmeB+CKtQkzp/n5ManA8Ex2JGGWIKpvtV9ce+OfK6e
// SIG // vaoxXlT9xmwLyW2N2xZaubssa4j3GcQ2awen9cAC16zt
// SIG // vyHX1RHcE1qiSA2QSY08nEIYcUt4tQIDAQABo4IBGzCC
// SIG // ARcwHQYDVR0OBBYEFIcy8GmfDgS0SR9LPROt6SHFiaJH
// SIG // MB8GA1UdIwQYMBaAFNVjOlyKMZDzQ3t8RhvFM2hahW1V
// SIG // MFYGA1UdHwRPME0wS6BJoEeGRWh0dHA6Ly9jcmwubWlj
// SIG // cm9zb2Z0LmNvbS9wa2kvY3JsL3Byb2R1Y3RzL01pY1Rp
// SIG // bVN0YVBDQV8yMDEwLTA3LTAxLmNybDBaBggrBgEFBQcB
// SIG // AQROMEwwSgYIKwYBBQUHMAKGPmh0dHA6Ly93d3cubWlj
// SIG // cm9zb2Z0LmNvbS9wa2kvY2VydHMvTWljVGltU3RhUENB
// SIG // XzIwMTAtMDctMDEuY3J0MAwGA1UdEwEB/wQCMAAwEwYD
// SIG // VR0lBAwwCgYIKwYBBQUHAwgwDQYJKoZIhvcNAQELBQAD
// SIG // ggEBAGYSlsGzJwvUMsHK5YLNKl/W4cYmRr3vvCRA9u4T
// SIG // oshmvEfZ6sx4OPqujSn2F30utrjFCadrvie7SDVE3/9b
// SIG // oC/iuEcLD1XoQ2rEiSY/u26CMWT/AFP8UHVWO6oJkBpn
// SIG // eBfnOYZCSbZejUXmBPdPEOvfKo9Zg6a9DMfuC4T/7U6i
// SIG // +h6WRFEZYRTnXZ8i0rVQhXXzSNchhz/Z9MjdSn4RhFd7
// SIG // OzAc6RSV8Dn5cIMhPXMEPI4zk1aTUXQqi/z+VYggKNnX
// SIG // Ih43dbtYtUOEGesW9PgdR3WRURCzohcH9LId2dcTnkhX
// SIG // B4NKyxPq7Hh8+EIb6BI+4fWw/FTe/x5wg8FqQSUwggZx
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
// SIG // UyBFU046QUUyQy1FMzJCLTFBRkMxJTAjBgNVBAMTHE1p
// SIG // Y3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2WiIwoBATAH
// SIG // BgUrDgMCGgMVAIcrgpa6HFn+EiHEWnwBF9UYbOs5oIGD
// SIG // MIGApH4wfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldh
// SIG // c2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNV
// SIG // BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UE
// SIG // AxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAw
// SIG // DQYJKoZIhvcNAQEFBQACBQDkbJ30MCIYDzIwMjEwNjEw
// SIG // MjIxOTMyWhgPMjAyMTA2MTEyMjE5MzJaMHcwPQYKKwYB
// SIG // BAGEWQoEATEvMC0wCgIFAORsnfQCAQAwCgIBAAICAoUC
// SIG // Af8wBwIBAAICEZ0wCgIFAORt73QCAQAwNgYKKwYBBAGE
// SIG // WQoEAjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQACAweh
// SIG // IKEKMAgCAQACAwGGoDANBgkqhkiG9w0BAQUFAAOBgQAD
// SIG // bOHGnLGbX4YjVUzZOeCe+zIAHok0DlmwWhnK22uR4H2E
// SIG // f4jIx6gDhmKx9NzLGk0p+dDcwSyOOC8+9Oc/SBEuhZqT
// SIG // aX/457fvB78F2fPlfSiSh1SaHXWyAB6vBO1ypkcDP1wx
// SIG // gbS9HcJsLtpLJMewFO/OuvwI8lyMkyilp7o8NzGCAw0w
// SIG // ggMJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
// SIG // BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
// SIG // MDEwAhMzAAABSKKIRVa8L4C/AAAAAAFIMA0GCWCGSAFl
// SIG // AwQCAQUAoIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcN
// SIG // AQkQAQQwLwYJKoZIhvcNAQkEMSIEIKse28boi1ckeCe0
// SIG // 5ZpZ9EzHUJzk0enP+236uPuE3kvaMIH6BgsqhkiG9w0B
// SIG // CRACLzGB6jCB5zCB5DCBvQQgqZAa6ox5ob8mH+bU3E7w
// SIG // +WOGvle/U8FVek9WgMHbaNgwgZgwgYCkfjB8MQswCQYD
// SIG // VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
// SIG // A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
// SIG // IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQg
// SIG // VGltZS1TdGFtcCBQQ0EgMjAxMAITMwAAAUiiiEVWvC+A
// SIG // vwAAAAABSDAiBCC/s8sTHdJbSGbzgxeuPkPtIPuHgBKi
// SIG // LvhlWWQV+RRRrjANBgkqhkiG9w0BAQsFAASCAQB0X6QW
// SIG // 9cjABrM+5oKpYuOkXulmKQpAUoewH6+EBhz1l/g5Stgt
// SIG // rPpZbRg4dryx9/G6dezjkY/AftclvEOFbqXENvkCpn/W
// SIG // s4eXuCaCYDVliey3jjibo6dnTENAOkNwKvQXBLwl9BL+
// SIG // xhSu5qUQgDbdP3r1bB8W6pjzX1qF6vvI5YG3GyKqnVWr
// SIG // ipNKRbk6BH4vKdatHkNqg0E6qvW5+JW1fjk44hcDjueQ
// SIG // WjyKMMmm/Dvz9a3yUB8iTMftNaaDecaDqCZDYARUos98
// SIG // I1RUnvACEvqC0RuK1DFj8AacOZkZU47sDbNxOX6ttYYw
// SIG // 8MPtk1D/5yyyTowVJ2O+EeVpMIKB
// SIG // End signature block
