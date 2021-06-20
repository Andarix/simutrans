
//
// FlipPolygons.js
//

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

function getMeshOrChildMesh(parent) {
    if (parent.typeId == "Microsoft.VisualStudio.3D.Mesh") {
        return parent;
    }
    return findFirstChildMeshElement(parent);
}

//
// this is used for undoing polygon mode flipping
//
function UndoableItem(collElem, meshElem) {

    this._clonedColl = collElem.clone();
    this._polyCollection = this._clonedColl.behavior;
    this._meshElem = meshElem;
    this._mesh = meshElem.behavior;

    var geom = this._meshElem.getTrait("Geometry").value;
    this._restoreGeom = geom.clone();

    this.getName = function () {
        var IDS_MreUndoFlipPolygons = 164;
        return services.strings.getStringFromId(IDS_MreUndoFlipPolygons);
    }

    this.onDo = function () {

        var geom = this._meshElem.getTrait("Geometry").value;

        this._mesh.selectedObjects = this._clonedColl.clone();

        var polyCount = this._polyCollection.getPolygonCount();
        for (var i = 0; i < polyCount; i++) {
            var polyIndex = this._polyCollection.getPolygon(i);
            
            // services.debug.trace("[FlipPolygons] flipping poly: " + polyIndex);

            //
            // flip the polygon
            //
            geom.flipPolygon(polyIndex);
        }

        this._mesh.recomputeCachedGeometry();
    }

    this.onUndo = function () {
        var geom = this._meshElem.getTrait("Geometry").value;
        geom.copyFrom(this._restoreGeom);

        this._mesh.selectedObjects = this._clonedColl.clone();

        this._mesh.recomputeCachedGeometry();
    }
}

//
// used for undoing object mode flipping
//
function UndoableObjectSelectionFlip(meshElems) {

    // in the object here
    this._meshElems = meshElems;
    this._restoreGeoms = new Array();

    // clone geometry we will use for restoring from undo
    for (var i = 0; i < this._meshElems.length; i++) {
        var geom = this._meshElems[i].getTrait("Geometry").value;
        this._restoreGeoms.push(geom.clone());
    }

    // name function 
    this.getName = function () {
        var IDS_MreUndoFlipPolygons = 164;
        return services.strings.getStringFromId(IDS_MreUndoFlipPolygons);
    }

    // do callback
    this.onDo = function () {

        for (var i = 0; i < this._meshElems.length; i++) {
            var geom = this._meshElems[i].getTrait("Geometry").value;

            // flip all polygons in the geom
            var polyCount = geom.polygonCount;
            for (var j = 0; j < polyCount; j++) {
                geom.flipPolygon(j);
            }

            this._meshElems[i].behavior.recomputeCachedGeometry();
        }
    }

    // undo callback
    this.onUndo = function () {

        for (var i = 0; i < this._meshElems.length; i++) {
            var geom = this._meshElems[i].getTrait("Geometry").value;

            geom.copyFrom(this._restoreGeoms[i]);

            this._meshElems[i].behavior.recomputeCachedGeometry();
        }
    }
}

var selectionMode = getSelectionMode();

//
// create the undoable object based on selection mode
//

if (selectionMode == 1) {

    // polygon selection mode

    // get the poly collection
    var selectedElement = document.selectedElement;
    var polyCollection = null;
    var mesh = null;
    var meshElem = null;
    var collElem = null;

    if (selectedElement != null) {
        meshElem = findFirstChildMeshElement(selectedElement);
        if (meshElem != null) {
            mesh = meshElem.behavior;

            // polygon selection mode
            collElem = mesh.selectedObjects;
            if (collElem != null) {
                polyCollection = collElem.behavior;
                if (polyCollection != null && collElem.typeId == "PolygonCollection") {
                    var undoableItem = new UndoableItem(collElem, meshElem);
                    undoableItem.onDo();
                    services.undoService.addUndoableItem(undoableItem);
                }
            }
        }
    }
}
else if (selectionMode == 0) {
    
    // object selection mode

    var meshes = new Array();

    // get the meshes we will flip polys on
    var count = services.selection.count;
    for (var i = 0; i < count; i++) {
        var currSelected = services.selection.getElement(i);
        var mesh = getMeshOrChildMesh(currSelected);
        if (mesh != null) {
            meshes.push(mesh);
        }
    }

    // if we have meshes, then create undoable item
    if (meshes.length > 0) {
        var undoableItem = new UndoableObjectSelectionFlip(meshes);
        undoableItem.onDo();
        services.undoService.addUndoableItem(undoableItem);
    }
}
// SIG // Begin signature block
// SIG // MIIjgAYJKoZIhvcNAQcCoIIjcTCCI20CAQExDzANBglg
// SIG // hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
// SIG // BgEEAYI3AgEeMCQCAQEEEBDgyQbOONQRoqMAEEvTUJAC
// SIG // AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
// SIG // Qn76216R4yQvnsnUTvXDBOVxjwFMUPtfwmzxcO4/0Meg
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
// SIG // SEXAQsmbdlsKgEhr/Xmfwb1tbWrJUnMTDXpQzTGCFVcw
// SIG // ghVTAgEBMIGVMH4xCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKDAm
// SIG // BgNVBAMTH01pY3Jvc29mdCBDb2RlIFNpZ25pbmcgUENB
// SIG // IDIwMTECEzMAAAHfa/AukqdKtNAAAAAAAd8wDQYJYIZI
// SIG // AWUDBAIBBQCgga4wGQYJKoZIhvcNAQkDMQwGCisGAQQB
// SIG // gjcCAQQwHAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcC
// SIG // ARUwLwYJKoZIhvcNAQkEMSIEIBO7gyOuvK/CKhs1X1vb
// SIG // YRKsNwHkmRj6c/xjiS5rvZWGMEIGCisGAQQBgjcCAQwx
// SIG // NDAyoBSAEgBNAGkAYwByAG8AcwBvAGYAdKEagBhodHRw
// SIG // Oi8vd3d3Lm1pY3Jvc29mdC5jb20wDQYJKoZIhvcNAQEB
// SIG // BQAEggEAcXbaYIdiKY6Yk3qbPzNUrYnaRVQ4RZvfliOo
// SIG // ftzBoldIKeXpTYrVFOoMnq240mkYSyTXu6/ITDJQ4o3p
// SIG // wK29UZ9c1vPl2k2GhLNWYNsOftWhK3a7/iul+X8mCtBn
// SIG // UUJh5T1Khb3CHjHEFaMDY0qYI2vYmxSbA4euTgsPVOsp
// SIG // Noc4MzaKSVF92Hc5/IOOqLj4H1yEeb8vsXw7PjGQD9u0
// SIG // SbsGmgycFHJmvhrPyIm1jgIKvJG8dbrxWUL8FhBfaREh
// SIG // +7SHQqg8O3RciL77V7qkoPCcYIz+j3h88qp9LuvI/r6G
// SIG // /pqVoYW/321qejEuqsNtUp9g8fZhwe3cGduxJlvVyKGC
// SIG // EuEwghLdBgorBgEEAYI3AwMBMYISzTCCEskGCSqGSIb3
// SIG // DQEHAqCCErowghK2AgEDMQ8wDQYJYIZIAWUDBAIBBQAw
// SIG // ggFQBgsqhkiG9w0BCRABBKCCAT8EggE7MIIBNwIBAQYK
// SIG // KwYBBAGEWQoDATAxMA0GCWCGSAFlAwQCAQUABCBpxcrF
// SIG // tHI/3EzUDGdOoGvpih+ELxUguSLn5OoFBB0FFQIGYK6X
// SIG // P8FKGBIyMDIxMDYxMTAwMjUyNS40NFowBIACAfSggdCk
// SIG // gc0wgcoxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
// SIG // aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
// SIG // ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNVBAsT
// SIG // HE1pY3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMxJjAk
// SIG // BgNVBAsTHVRoYWxlcyBUU1MgRVNOOkQ2QkQtRTNFNy0x
// SIG // Njg1MSUwIwYDVQQDExxNaWNyb3NvZnQgVGltZS1TdGFt
// SIG // cCBTZXJ2aWNloIIOOTCCBPEwggPZoAMCAQICEzMAAAFQ
// SIG // WKLUp5sLMOsAAAAAAVAwDQYJKoZIhvcNAQELBQAwfDEL
// SIG // MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
// SIG // EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jv
// SIG // c29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWljcm9z
// SIG // b2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwHhcNMjAxMTEy
// SIG // MTgyNjAzWhcNMjIwMjExMTgyNjAzWjCByjELMAkGA1UE
// SIG // BhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNV
// SIG // BAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBD
// SIG // b3Jwb3JhdGlvbjElMCMGA1UECxMcTWljcm9zb2Z0IEFt
// SIG // ZXJpY2EgT3BlcmF0aW9uczEmMCQGA1UECxMdVGhhbGVz
// SIG // IFRTUyBFU046RDZCRC1FM0U3LTE2ODUxJTAjBgNVBAMT
// SIG // HE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2UwggEi
// SIG // MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDnen+U
// SIG // eypZwycbVpoN8zNSAqnZl40+RjRTx17gsPvVYNxvPe6P
// SIG // zruS/J5X2mON6BRt+XaJATJJvkCgHvViJqrU7Q39T0qT
// SIG // f02fOTTzkBR1zhB2ihL3XSaEpRE/L2wSa7vgL8jhPFi0
// SIG // dZ8nnqcj96bVLaRvPs7ANXeDF3xpZNgUSKL2EegBcmRU
// SIG // se+92uWk/NYsj8Y3ECv2qPnSCNESqdQ97JS4K3R5PzHS
// SIG // CG2xYvRRLp+b90FVI2JCQr1IAj92UNke2wKHbQs5VdyJ
// SIG // E+/vgg6tyZdaxW7AVojIq5KcfM3+QahNKpsdOHm37IwY
// SIG // mD1LfTsb0tVhXLjbh7o4T6cCKiWbAgMBAAGjggEbMIIB
// SIG // FzAdBgNVHQ4EFgQUglUZHxlF261kL0PBAEM7t+ufRX4w
// SIG // HwYDVR0jBBgwFoAU1WM6XIoxkPNDe3xGG8UzaFqFbVUw
// SIG // VgYDVR0fBE8wTTBLoEmgR4ZFaHR0cDovL2NybC5taWNy
// SIG // b3NvZnQuY29tL3BraS9jcmwvcHJvZHVjdHMvTWljVGlt
// SIG // U3RhUENBXzIwMTAtMDctMDEuY3JsMFoGCCsGAQUFBwEB
// SIG // BE4wTDBKBggrBgEFBQcwAoY+aHR0cDovL3d3dy5taWNy
// SIG // b3NvZnQuY29tL3BraS9jZXJ0cy9NaWNUaW1TdGFQQ0Ff
// SIG // MjAxMC0wNy0wMS5jcnQwDAYDVR0TAQH/BAIwADATBgNV
// SIG // HSUEDDAKBggrBgEFBQcDCDANBgkqhkiG9w0BAQsFAAOC
// SIG // AQEAUT9odHKO/uPj08AeL5P2HixMOqHK3oPk9JAdmlgf
// SIG // 2Xt8xF7Y9BHiFQNWYMKd/HI2ryYOu3SAAs3txZaRpalv
// SIG // Y0R16WWIQzC9G9oqSD7QNN0RMxsiiCMM65/nq9xSPIrm
// SIG // Yh6aTXFgIMuh4GLNk7gMQFybUbg2ZlLZsn9r5RzxX/x8
// SIG // aK17ggEWKmiij1lgb/6AE+bAPUuEyy50ua6U9Zs0+bi8
// SIG // /HvnZs6PiMwGhtXz/sRrZaAYjbLvaCXOk+DbRvHBoYHQ
// SIG // Qm35QrPUIfiNcw30giIMRy7xYHjiml/IxakMFUJ56mLE
// SIG // 3SvnbSGxaKwppPlkIsw5HhemdSGHs5SlrQTbXjCCBnEw
// SIG // ggRZoAMCAQICCmEJgSoAAAAAAAIwDQYJKoZIhvcNAQEL
// SIG // BQAwgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
// SIG // aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
// SIG // ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xMjAwBgNVBAMT
// SIG // KU1pY3Jvc29mdCBSb290IENlcnRpZmljYXRlIEF1dGhv
// SIG // cml0eSAyMDEwMB4XDTEwMDcwMTIxMzY1NVoXDTI1MDcw
// SIG // MTIxNDY1NVowfDELMAkGA1UEBhMCVVMxEzARBgNVBAgT
// SIG // Cldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAc
// SIG // BgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQG
// SIG // A1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIw
// SIG // MTAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB
// SIG // AQCpHQ28dxGKOiDs/BOX9fp/aZRrdFQQ1aUKAIKF++18
// SIG // aEssX8XD5WHCdrc+Zitb8BVTJwQxH0EbGpUdzgkTjnxh
// SIG // MFmxMEQP8WCIhFRDDNdNuDgIs0Ldk6zWczBXJoKjRQ3Q
// SIG // 6vVHgc2/JGAyWGBG8lhHhjKEHnRhZ5FfgVSxz5NMksHE
// SIG // pl3RYRNuKMYa+YaAu99h/EbBJx0kZxJyGiGKr0tkiVBi
// SIG // sV39dx898Fd1rL2KQk1AUdEPnAY+Z3/1ZsADlkR+79BL
// SIG // /W7lmsqxqPJ6Kgox8NpOBpG2iAg16HgcsOmZzTznL0S6
// SIG // p/TcZL2kAcEgCZN4zfy8wMlEXV4WnAEFTyJNAgMBAAGj
// SIG // ggHmMIIB4jAQBgkrBgEEAYI3FQEEAwIBADAdBgNVHQ4E
// SIG // FgQU1WM6XIoxkPNDe3xGG8UzaFqFbVUwGQYJKwYBBAGC
// SIG // NxQCBAweCgBTAHUAYgBDAEEwCwYDVR0PBAQDAgGGMA8G
// SIG // A1UdEwEB/wQFMAMBAf8wHwYDVR0jBBgwFoAU1fZWy4/o
// SIG // olxiaNE9lJBb186aGMQwVgYDVR0fBE8wTTBLoEmgR4ZF
// SIG // aHR0cDovL2NybC5taWNyb3NvZnQuY29tL3BraS9jcmwv
// SIG // cHJvZHVjdHMvTWljUm9vQ2VyQXV0XzIwMTAtMDYtMjMu
// SIG // Y3JsMFoGCCsGAQUFBwEBBE4wTDBKBggrBgEFBQcwAoY+
// SIG // aHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraS9jZXJ0
// SIG // cy9NaWNSb29DZXJBdXRfMjAxMC0wNi0yMy5jcnQwgaAG
// SIG // A1UdIAEB/wSBlTCBkjCBjwYJKwYBBAGCNy4DMIGBMD0G
// SIG // CCsGAQUFBwIBFjFodHRwOi8vd3d3Lm1pY3Jvc29mdC5j
// SIG // b20vUEtJL2RvY3MvQ1BTL2RlZmF1bHQuaHRtMEAGCCsG
// SIG // AQUFBwICMDQeMiAdAEwAZQBnAGEAbABfAFAAbwBsAGkA
// SIG // YwB5AF8AUwB0AGEAdABlAG0AZQBuAHQALiAdMA0GCSqG
// SIG // SIb3DQEBCwUAA4ICAQAH5ohRDeLG4Jg/gXEDPZ2joSFv
// SIG // s+umzPUxvs8F4qn++ldtGTCzwsVmyWrf9efweL3HqJ4l
// SIG // 4/m87WtUVwgrUYJEEvu5U4zM9GASinbMQEBBm9xcF/9c
// SIG // +V4XNZgkVkt070IQyK+/f8Z/8jd9Wj8c8pl5SpFSAK84
// SIG // Dxf1L3mBZdmptWvkx872ynoAb0swRCQiPM/tA6WWj1kp
// SIG // vLb9BOFwnzJKJ/1Vry/+tuWOM7tiX5rbV0Dp8c6ZZpCM
// SIG // /2pif93FSguRJuI57BlKcWOdeyFtw5yjojz6f32WapB4
// SIG // pm3S4Zz5Hfw42JT0xqUKloakvZ4argRCg7i1gJsiOCC1
// SIG // JeVk7Pf0v35jWSUPei45V3aicaoGig+JFrphpxHLmtgO
// SIG // R5qAxdDNp9DvfYPw4TtxCd9ddJgiCGHasFAeb73x4QDf
// SIG // 5zEHpJM692VHeOj4qEir995yfmFrb3epgcunCaw5u+zG
// SIG // y9iCtHLNHfS4hQEegPsbiSpUObJb2sgNVZl6h3M7COaY
// SIG // LeqN4DMuEin1wC9UJyH3yKxO2ii4sanblrKnQqLJzxlB
// SIG // TeCG+SqaoxFmMNO7dDJL32N79ZmKLxvHIa9Zta7cRDyX
// SIG // UHHXodLFVeNp3lfB0d4wwP3M5k37Db9dT+mdHhk4L7zP
// SIG // WAUu7w2gUDXa7wknHNWzfjUeCLraNtvTX4/edIhJEqGC
// SIG // AsswggI0AgEBMIH4oYHQpIHNMIHKMQswCQYDVQQGEwJV
// SIG // UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
// SIG // UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
// SIG // cmF0aW9uMSUwIwYDVQQLExxNaWNyb3NvZnQgQW1lcmlj
// SIG // YSBPcGVyYXRpb25zMSYwJAYDVQQLEx1UaGFsZXMgVFNT
// SIG // IEVTTjpENkJELUUzRTctMTY4NTElMCMGA1UEAxMcTWlj
// SIG // cm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaIjCgEBMAcG
// SIG // BSsOAwIaAxUAIw17n3LxNWtGEZtallmkMZYeoBKggYMw
// SIG // gYCkfjB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQD
// SIG // Ex1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMDAN
// SIG // BgkqhkiG9w0BAQUFAAIFAORs28QwIhgPMjAyMTA2MTEw
// SIG // MjQzMTZaGA8yMDIxMDYxMjAyNDMxNlowdDA6BgorBgEE
// SIG // AYRZCgQBMSwwKjAKAgUA5GzbxAIBADAHAgEAAgIBozAH
// SIG // AgEAAgIRdzAKAgUA5G4tRAIBADA2BgorBgEEAYRZCgQC
// SIG // MSgwJjAMBgorBgEEAYRZCgMCoAowCAIBAAIDB6EgoQow
// SIG // CAIBAAIDAYagMA0GCSqGSIb3DQEBBQUAA4GBAFnd+E3W
// SIG // HXn6H110IRAono5lstdjc0eeEYMjJute5bp6E3C79dcW
// SIG // Py4+USvRkboj6OLwzjIX+4/ZXgN/jzE76MhnwKElgTZS
// SIG // +7TTOSbklQpxjcbjfOKO1tMyixqt5yGSa5galzMPeVek
// SIG // c7EAA1+EgszKqX7+Iu+0NUjgepCuoW1lMYIDDTCCAwkC
// SIG // AQEwgZMwfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldh
// SIG // c2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNV
// SIG // BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UE
// SIG // AxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAC
// SIG // EzMAAAFQWKLUp5sLMOsAAAAAAVAwDQYJYIZIAWUDBAIB
// SIG // BQCgggFKMBoGCSqGSIb3DQEJAzENBgsqhkiG9w0BCRAB
// SIG // BDAvBgkqhkiG9w0BCQQxIgQgRPNBcxQQq1bfCvRhDXFO
// SIG // erjpncodn8w3QSsy3BIHFskwgfoGCyqGSIb3DQEJEAIv
// SIG // MYHqMIHnMIHkMIG9BCBs9D6fL5rCThgXJmGIhdXS6IY1
// SIG // Zg6op47dkKJ8L/Kj9jCBmDCBgKR+MHwxCzAJBgNVBAYT
// SIG // AlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQH
// SIG // EwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29y
// SIG // cG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1l
// SIG // LVN0YW1wIFBDQSAyMDEwAhMzAAABUFii1KebCzDrAAAA
// SIG // AAFQMCIEIKLeYUccf3dp8LchRNz37kcY5axpRQHZtvnD
// SIG // MyeBFyF4MA0GCSqGSIb3DQEBCwUABIIBAFahaSK1p68q
// SIG // GR4NKa19s6aGUgEbU1k2FkG3JrP+87CvN4fFZwC9iJ29
// SIG // 5vJExq8VBsSlFdieeJwGT6sgibvbfDYbFK+2FNPQsTuL
// SIG // KaPR2+pm4l7OlcM+wRs4r3o1uvrGru/8vnh5RHX3pnF9
// SIG // eKhPY5tW/L+0QtG3uvYv1HYsFFvzxVFTWvk03+2qHbvv
// SIG // so7WbmNqIta/pt4ra62qYcR4RNgfU8kYDMwI5fzMspQq
// SIG // JL6LXrXPt8B2DmgdPxK2kHLCXDVBjYw9FwtnRr8xYB53
// SIG // 9pTRo0IsVpau5tDi/TI320Tiz8xBBZ42h4KEaOuPYx/+
// SIG // c2ndf5Qqwbm/6nexF+cn4Kc=
// SIG // End signature block
