﻿

//
// helper to get a designer property as a bool
//
function getDesignerPropAsBool(tname) {
    if (document.designerProps.hasTrait(tname))
        return document.designerProps.getTrait(tname).value;

    return false;
}

//
// helper to get the selection mode
//
function getSelectionMode() {
    if (getDesignerPropAsBool("usePivot"))
        return 0; // default to object mode when using pivot
    if (document.designerProps.hasTrait("SelectionMode"))
        return document.designerProps.getTrait("SelectionMode").value;
    return 0;
}

//
// helper to find the first mesh child of a scene node
//
function findFirstChildMeshElement(parent) {
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

//
// undoable item
//
// does the bulk of the "create object from polygon selection" command work
//
function UndoableItem(collElem, meshElem) {

    var clonedColl = collElem.clone();

    this._polyCollection = clonedColl.behavior;
    this._sourceMeshElem = meshElem;
    this._sourceMesh = meshElem.behavior;
    this._createdMeshElem = null;
    this._createdNode = null;

    var owningSceneNode = this._sourceMeshElem.parent;
    this._parentOfSceneNode = owningSceneNode.parent;

    var geom = this._sourceMeshElem.getTrait("Geometry").value;
    this._restoreGeom = geom.clone();

    this.getName = function () {
        var IDS_MreUndoCreateObjectFromPolygonSelection = 153;
        return services.strings.getStringFromId(IDS_MreUndoCreateObjectFromPolygonSelection);
    }

    this.onDo = function () {

        //
        // create a new mesh from selection
        //
        if (this._createdMeshElem == null) {
            this._createdMeshElem = this._polyCollection.createMeshFromPolygons();
            document.elements.append(this._createdMeshElem);

            this._createdNode = document.createSceneNode("PolygonSelection");

            // take the transform of mesh nodes parent element
            this._createdNode.behavior.copyTransformData(this._sourceMeshElem.parent.behavior);

            this._createdMeshElem.parent = this._createdNode;
        }
        else {
            this._createdMeshElem.parent = this._createdNode;

            document.elements.append(this._createdMeshElem);
            document.elements.append(this._createdNode);
        }

        // set parent of the scene node that owns our  mesh
        // to the parent of the scene node that owns the orig mesh
        this._createdNode.parent = this._parentOfSceneNode;


        //
        // now delete the polygons we've created new geometry from
        //

        var geom = this._sourceMeshElem.getTrait("Geometry").value;

        // delete polygons from the source geometry
        var polysToDelete = new Array();
        var polyCount = this._polyCollection.getPolygonCount();
        for (var i = 0; i < polyCount; i++) {
            var polyIndex = this._polyCollection.getPolygon(i);
            polysToDelete.push(polyIndex);
        }

        // sort then delete in order
        function sortNumberDescending(a, b) {
            return b - a;
        }

        polysToDelete.sort(sortNumberDescending);

        for (var p = 0; p < polysToDelete.length; p++) {
            geom.removePolygon(polysToDelete[p]);
        }

        // fix up geometry
        this._sourceMesh.selectedObjects = null;
        this._sourceMesh.recomputeCachedGeometry();
        this._sourceMesh.computeNormals();
    }

    this.onUndo = function () {

        // delete the created object
        document.deleteSceneElement(this._createdNode);

        // restore the original geometry since we've removed polygons from it
        var geom = this._sourceMeshElem.getTrait("Geometry").value;
        geom.copyFrom(this._restoreGeom);

        // invalidate geometry
        this._sourceMesh.recomputeCachedGeometry();
    }
}

//
// create an undoable item based on polygon collection
//
var selectedElement = document.selectedElement;
var selectionMode = getSelectionMode();

// get the poly collection
var polyCollection = null;
var mesh = null;
var meshElem = null;
var collElem = null;
if (selectedElement != null) {
    // if polygon selection mode...
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
// SIG // b8e/rSwS0qb33xhMEU6U3JLc7dMs8U1ptwN3SKd4BrCg
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
// SIG // ARUwLwYJKoZIhvcNAQkEMSIEIDpInPcYa8fz50WQJY/k
// SIG // RPpS8F0PjeI3c2+IQRCHhC3+MEIGCisGAQQBgjcCAQwx
// SIG // NDAyoBSAEgBNAGkAYwByAG8AcwBvAGYAdKEagBhodHRw
// SIG // Oi8vd3d3Lm1pY3Jvc29mdC5jb20wDQYJKoZIhvcNAQEB
// SIG // BQAEggEAbeIyO6X8Guj5bOpXt7cuwcWGPFiiAjjnr9q6
// SIG // KvjZG6QJN0hyJWEuEbairERfYFTtoewiOAdEdiv4cLsM
// SIG // 5fUKGDfRDCTBNsSBm55/K0N6+JGGgXnf0r6jxQlOV1Oh
// SIG // FTKicAWng6zL/eSizKX2qUsX9wcGI83dQQp6a2yCLJ4b
// SIG // WubDvPexaU4XDLpJVg67SR54eKZ1L8M4HLExTsaLnfCA
// SIG // dZrwlW09oTUjY0deJMtJRgJkLClYWUYmR/ATK2mYbObK
// SIG // rEK1Zfq+bfs/HbTndwpgIvNNDVd9uqMcQy1T3SQFpVVm
// SIG // 6ZhwYF8rTA4ex+zJdfbursKyBqs1gjUnRhqe9ykzPKGC
// SIG // EuUwghLhBgorBgEEAYI3AwMBMYIS0TCCEs0GCSqGSIb3
// SIG // DQEHAqCCEr4wghK6AgEDMQ8wDQYJYIZIAWUDBAIBBQAw
// SIG // ggFRBgsqhkiG9w0BCRABBKCCAUAEggE8MIIBOAIBAQYK
// SIG // KwYBBAGEWQoDATAxMA0GCWCGSAFlAwQCAQUABCB5oLPq
// SIG // mYR5ySdgpH1sJcbFjCVX0vnV3wD/V97yM2bQ+wIGYK6Z
// SIG // 0qC6GBMyMDIxMDYxMTAwMjUyNS40NjlaMASAAgH0oIHQ
// SIG // pIHNMIHKMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSUwIwYDVQQL
// SIG // ExxNaWNyb3NvZnQgQW1lcmljYSBPcGVyYXRpb25zMSYw
// SIG // JAYDVQQLEx1UaGFsZXMgVFNTIEVTTjo3QkYxLUUzRUEt
// SIG // QjgwODElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUtU3Rh
// SIG // bXAgU2VydmljZaCCDjwwggTxMIID2aADAgECAhMzAAAB
// SIG // UcNQ51lsqsanAAAAAAFRMA0GCSqGSIb3DQEBCwUAMHwx
// SIG // CzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9u
// SIG // MRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNy
// SIG // b3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jv
// SIG // c29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMB4XDTIwMTEx
// SIG // MjE4MjYwNFoXDTIyMDIxMTE4MjYwNFowgcoxCzAJBgNV
// SIG // BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
// SIG // VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
// SIG // Q29ycG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBB
// SIG // bWVyaWNhIE9wZXJhdGlvbnMxJjAkBgNVBAsTHVRoYWxl
// SIG // cyBUU1MgRVNOOjdCRjEtRTNFQS1CODA4MSUwIwYDVQQD
// SIG // ExxNaWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNlMIIB
// SIG // IjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAn9KH
// SIG // 76qErjvvOIkjWbHptMkYDjmG+JEmzguyr/VxjZgZ/ig8
// SIG // Mk47jqSJP5RxH/sDyqhYu7jPSO86siZh8u7DBX9L8I+A
// SIG // B+8fPPvD4uoLKD22BpoFl4B8Fw5K7SuibvbxGN7adL1/
// SIG // zW+sWXlVvpDhEPIKDICvEdNjGTLhktfftjefg9lumBMU
// SIG // BJ2G4/g4ad0dDvRNmKiMZXXe/Ll4Qg/oPSzXCUEYoSSq
// SIG // a5D+5MRimVe5/YTLj0jVr8iF45V0hT7VH8OJO4YImcnZ
// SIG // hq6Dw1G+w6ACRGePFmOWqW8tEZ13SMmOquJrTkwyy8zy
// SIG // NtVttJAX7diFLbR0SvMlbJZWK0KHdwIDAQABo4IBGzCC
// SIG // ARcwHQYDVR0OBBYEFMV3/+NoUGKTNGg6OMyE6fN1ROpt
// SIG // MB8GA1UdIwQYMBaAFNVjOlyKMZDzQ3t8RhvFM2hahW1V
// SIG // MFYGA1UdHwRPME0wS6BJoEeGRWh0dHA6Ly9jcmwubWlj
// SIG // cm9zb2Z0LmNvbS9wa2kvY3JsL3Byb2R1Y3RzL01pY1Rp
// SIG // bVN0YVBDQV8yMDEwLTA3LTAxLmNybDBaBggrBgEFBQcB
// SIG // AQROMEwwSgYIKwYBBQUHMAKGPmh0dHA6Ly93d3cubWlj
// SIG // cm9zb2Z0LmNvbS9wa2kvY2VydHMvTWljVGltU3RhUENB
// SIG // XzIwMTAtMDctMDEuY3J0MAwGA1UdEwEB/wQCMAAwEwYD
// SIG // VR0lBAwwCgYIKwYBBQUHAwgwDQYJKoZIhvcNAQELBQAD
// SIG // ggEBACv99cAVg5nx0SqjvLfQzmugMj5cJ9NE60duSH1L
// SIG // pxHYim9Ls3UfiYd7t0JvyEw/rRTEKHbznV6LFLlX++lH
// SIG // JMGKzZnHtTe2OI6ZHFnNiFhtgyWuYDJrm7KQykNi1G1L
// SIG // buVie9MehmoK+hBiZnnrcfZSnBSokrvO2QEWHC1xnZ5w
// SIG // M82UEjprFYOkchU+6RcoCjjmIFGfgSzNj1MIbf4lcJ5F
// SIG // oV1Mg6FwF45CijOXHVXrzkisMZ9puDpFjjEV6TAY6INg
// SIG // MkhLev/AVow0sF8MfQztJIlFYdFEkZ5NF/IyzoC2Yb9i
// SIG // w4bCKdBrdD3As6mvoGSNjCC6lOdz6EerJK3NhFgwggZx
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
// SIG // UyBFU046N0JGMS1FM0VBLUI4MDgxJTAjBgNVBAMTHE1p
// SIG // Y3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2WiIwoBATAH
// SIG // BgUrDgMCGgMVAKCir3PxP6RCCyVMJSAVoMV61yNeoIGD
// SIG // MIGApH4wfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldh
// SIG // c2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNV
// SIG // BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UE
// SIG // AxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAw
// SIG // DQYJKoZIhvcNAQEFBQACBQDkbN5aMCIYDzIwMjEwNjEx
// SIG // MDI1NDE4WhgPMjAyMTA2MTIwMjU0MThaMHcwPQYKKwYB
// SIG // BAGEWQoEATEvMC0wCgIFAORs3loCAQAwCgIBAAICBNAC
// SIG // Af8wBwIBAAICEXUwCgIFAORuL9oCAQAwNgYKKwYBBAGE
// SIG // WQoEAjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQACAweh
// SIG // IKEKMAgCAQACAwGGoDANBgkqhkiG9w0BAQUFAAOBgQBI
// SIG // MgQQxiI6+QNwbhHkld5Pea256XqO6OiWBehjKsF6DOUL
// SIG // eXPwOmry+BE20BP0ViuiWMaYFPKTCVT6NpLpWDCJCQOP
// SIG // EvF6lgTINqngGSK9PCh9TO4YhFL82jUBsVn23CfYwLv1
// SIG // WySyIW0D7eDVjIsZVEUpCpVuHsZrxpk+7qQGIDGCAw0w
// SIG // ggMJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
// SIG // BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
// SIG // MDEwAhMzAAABUcNQ51lsqsanAAAAAAFRMA0GCWCGSAFl
// SIG // AwQCAQUAoIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcN
// SIG // AQkQAQQwLwYJKoZIhvcNAQkEMSIEIJq5FdZEY2EShL0Q
// SIG // pfc4QlHlEags/RJYNlwp4fjadJyFMIH6BgsqhkiG9w0B
// SIG // CRACLzGB6jCB5zCB5DCBvQQgLs1cmYj41sFZBwCmFvv9
// SIG // ScP5tuuUhxsv/t0B9XF65UEwgZgwgYCkfjB8MQswCQYD
// SIG // VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
// SIG // A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
// SIG // IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQg
// SIG // VGltZS1TdGFtcCBQQ0EgMjAxMAITMwAAAVHDUOdZbKrG
// SIG // pwAAAAABUTAiBCDopuIVF+1kkPlqZc2V3jLE+Ubg/ziC
// SIG // 3LVtDZj8PWE/QTANBgkqhkiG9w0BAQsFAASCAQBQ+KfC
// SIG // 3VjTFfZSKm3S5M2KUweyRHdh8UL35mv0XgRt27e7HPRI
// SIG // xMziYHGG9AQo0SPq+VBKGsmmxrUCf0VuF0225/1uEPT+
// SIG // qrDjxmwYSlkVeXzbFvjuVPZAtJAUpDalcCXlQ8VoX7FF
// SIG // loWMdICaCqIO9yiOTpDRFX+3mmZT9z5V28fOGgcBYv9p
// SIG // mcKYFcIYRm1NsRvEePiJJX2ydgEaZQxK1f1OyK4EUZj4
// SIG // bQgLG6TE4116t3sMzCjJbz0N0Qp6h1p422dXaX2Nu1Tw
// SIG // L1O5cZOG/JjjTFyjPWfiejJhuLSvE2KqjWz+VfkoMejA
// SIG // ElSza5CkuzW8vKigJMLYTsOS1eLS
// SIG // End signature block
