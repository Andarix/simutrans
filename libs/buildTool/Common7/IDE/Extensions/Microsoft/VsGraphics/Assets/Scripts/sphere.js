
//
// create a sphere in the scene
//

// enable in prop window
var flags = 0x8;

// create the mesh and scene node and place into documents list
var sphereElement = document.createMesh(1102);
var mesh = sphereElement.behavior;

var material = services.effects.createEffectInstance("Phong");

// set up the color traits
var diffuseColorTrait = material.getOrCreateTrait("MaterialDiffuse", "float4", flags);
diffuseColorTrait.value = [1, 1, 1, 1];

var ambientColorTrait = material.getOrCreateTrait("MaterialAmbient", "float4", flags);
ambientColorTrait.value = [1, 1, 1, 1]

// add to our materials collection
mesh.materials.append(material);

// get the geometry
var geom = sphereElement.getTrait("Geometry").value;

function generateSpherePoints(radius, hDiv, vDiv) {
    var pointList = new Array();

    // Points in between
    for (var v = 0; v <= vDiv; v ++)
    {
        for (var h = 0; h < hDiv; h++) {
            var hAngle = h * (2 * Math.PI) / vDiv;
            var vAngle = v * Math.PI / hDiv;

            var x = radius * Math.cos(hAngle) * Math.sin(vAngle);
            var y = radius * Math.cos(vAngle);
            var z = radius * Math.sin(hAngle) * Math.sin(vAngle);
            pointList.push(x,y,z);
        }
    }
    
    return pointList;
}

var hDivisions = 22;
var vDivisions = 22;

// Sphere points
var points = generateSpherePoints(1, hDivisions, vDivisions);
var pointCount = points.length / 3;

// update the geometry
geom.addPoints(points, pointCount);

var polygonPointCounts = new Array();
var polygonCount = (hDivisions) * (vDivisions);
for (var i = 0; i < polygonCount; i++)
{
    polygonPointCounts.push(4);
}

var polygonPointIndices = new Array();
for (var i = 0; i < vDivisions; i++) {
    for (var j = 0; j < hDivisions; j++) {
        var nextH = j + 1;
        if (nextH == hDivisions) {
            nextH = 0;
        }
        var row0 = i * hDivisions;
        var row1 = (i+1) * hDivisions;
        polygonPointIndices.push(row1 + nextH, row1 + j, row0 + j, row0 + nextH);
    }
}

// this uses material '0' 
geom.addPolygons(0, polygonPointIndices, polygonPointCounts, polygonCount);

function generateSphereUV(hDiv, vDiv) {
    var uvs = new Array();

    // Points in between
    for (var v = 0.0; v <= vDiv; v += 1.0) {
        for (var h = 0.0; h <= hDiv; h += 1.0) {

            var x = h / vDiv;
            var y = v / hDiv;

            var uv = new Object();
            uv.x = 1 - x;
            uv.y = 1 - y;
            uvs.push(uv);
        }
    }

    return uvs;
}

var uvs = generateSphereUV(hDivisions, vDivisions);
var IndexingModePerPointOnPoly = 3;

var splitUVs = new Array();
for (var i = 0; i < vDivisions; i++) {
    for (var j = 0; j < hDivisions; j++) {
        var nextH = j + 1;

        var row0 = i * (hDivisions+1);
        var row1 = (i + 1) * (hDivisions+1);
        splitUVs.push(uvs[row1 + nextH].x, uvs[row1 + nextH].y);
        splitUVs.push(uvs[row1 + j].x, uvs[row1 + j].y);
        splitUVs.push(uvs[row0 + j].x, uvs[row0 + j].y);
        splitUVs.push(uvs[row0 + nextH].x, uvs[row0 + nextH].y);
    }
}

geom.addTextureCoordinates(splitUVs, splitUVs.length / 2);
geom.textureCoordinateIndexingMode = IndexingModePerPointOnPoly;

var coord = document.getCoordinateSystemMatrix();
geom.transform(coord);

sphereElement.getTrait("SmoothingAngle").value = 45;
var mesh = sphereElement.behavior;
mesh.computeNormals();

//
// create an undoable operation that creates the object on do and deletes the object on undo
//
function UndoableItem(element, parent) {
    this._element = element;
    this._parentElement = parent;

    this.getName = function () {
        var IDS_MreUndoCreateSphere = 157;
        return services.strings.getStringFromId(IDS_MreUndoCreateSphere);
    }

    this.onDo = function () {
        this._element.parent = this._parentElement;
        document.elements.append(this._parentElement);
        document.elements.append(this._element);

        this._element.parent = this._parentElement;
        this._parentElement.parent = document.getSceneRoot();
    }

    this.onUndo = function () {
        document.deleteSceneElement(this._parentElement);
    }
}

undoableItem = new UndoableItem(sphereElement, sphereElement.parent);
services.undoService.addUndoableItem(undoableItem);
// SIG // Begin signature block
// SIG // MIIjgwYJKoZIhvcNAQcCoIIjdDCCI3ACAQExDzANBglg
// SIG // hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
// SIG // BgEEAYI3AgEeMCQCAQEEEBDgyQbOONQRoqMAEEvTUJAC
// SIG // AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
// SIG // fkP4j+hGT8TDZ2McMlOXJmobgDiJjiYdweB/DxqZQ4ig
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
// SIG // SEXAQsmbdlsKgEhr/Xmfwb1tbWrJUnMTDXpQzTGCFVow
// SIG // ghVWAgEBMIGVMH4xCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKDAm
// SIG // BgNVBAMTH01pY3Jvc29mdCBDb2RlIFNpZ25pbmcgUENB
// SIG // IDIwMTECEzMAAAHfa/AukqdKtNAAAAAAAd8wDQYJYIZI
// SIG // AWUDBAIBBQCgga4wGQYJKoZIhvcNAQkDMQwGCisGAQQB
// SIG // gjcCAQQwHAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcC
// SIG // ARUwLwYJKoZIhvcNAQkEMSIEIGWvEFVx9YeRP5S2FUha
// SIG // m3s8YKFJ9CrB1rA9PlX7F907MEIGCisGAQQBgjcCAQwx
// SIG // NDAyoBSAEgBNAGkAYwByAG8AcwBvAGYAdKEagBhodHRw
// SIG // Oi8vd3d3Lm1pY3Jvc29mdC5jb20wDQYJKoZIhvcNAQEB
// SIG // BQAEggEATYIZXImbXAS+6V4Ctj41JoYzT80nhiL4M/+N
// SIG // AnDgPTbIwajx3hWKSoNZckthZBjwIp8Mng+Mt29cQa6W
// SIG // B4GzwibX3cixEO3bSALVwp3GHjCMEnwNTOui5kC1aArQ
// SIG // Ze4l1Md0pyuqDcBO5/nm5B7+a/tOfSqFSwJTmJCVvo8U
// SIG // tVil67sxaEL5a5vESP3t11WzPKIdAvmEk/ME5HCHwjrg
// SIG // bjTcSlP5x7fvNdXmiLvb2pypJ0Z8dsbOljFu6WC+xemr
// SIG // wXxAk4yncp64XQq4n3G4emd/K8RJ5KASL8DWAd/L5Hne
// SIG // Yg48XPYgVJqLzf5s82VzVRatQMHg21M/9DoaN8AQXaGC
// SIG // EuQwghLgBgorBgEEAYI3AwMBMYIS0DCCEswGCSqGSIb3
// SIG // DQEHAqCCEr0wghK5AgEDMQ8wDQYJYIZIAWUDBAIBBQAw
// SIG // ggFQBgsqhkiG9w0BCRABBKCCAT8EggE7MIIBNwIBAQYK
// SIG // KwYBBAGEWQoDATAxMA0GCWCGSAFlAwQCAQUABCCN50ae
// SIG // 4pjTJLIZ4FGyGbRsAmwMLG/EfXkTRFyvAIFzkgIGYK5Z
// SIG // XO/SGBIyMDIxMDYxMTAwMjUyNC43NlowBIACAfSggdCk
// SIG // gc0wgcoxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
// SIG // aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
// SIG // ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNVBAsT
// SIG // HE1pY3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMxJjAk
// SIG // BgNVBAsTHVRoYWxlcyBUU1MgRVNOOkFFMkMtRTMyQi0x
// SIG // QUZDMSUwIwYDVQQDExxNaWNyb3NvZnQgVGltZS1TdGFt
// SIG // cCBTZXJ2aWNloIIOPDCCBPEwggPZoAMCAQICEzMAAAFI
// SIG // oohFVrwvgL8AAAAAAUgwDQYJKoZIhvcNAQELBQAwfDEL
// SIG // MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
// SIG // EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jv
// SIG // c29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWljcm9z
// SIG // b2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwHhcNMjAxMTEy
// SIG // MTgyNTU2WhcNMjIwMjExMTgyNTU2WjCByjELMAkGA1UE
// SIG // BhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNV
// SIG // BAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBD
// SIG // b3Jwb3JhdGlvbjElMCMGA1UECxMcTWljcm9zb2Z0IEFt
// SIG // ZXJpY2EgT3BlcmF0aW9uczEmMCQGA1UECxMdVGhhbGVz
// SIG // IFRTUyBFU046QUUyQy1FMzJCLTFBRkMxJTAjBgNVBAMT
// SIG // HE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2UwggEi
// SIG // MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQD3/3iv
// SIG // FYSK0dGtcXaZ8pNLEARbraJewryi/JgbaKlq7hhFIU1E
// SIG // kY0HMiFRm2/Wsukt62k25zvDxW16fphg5876+l1wYnCl
// SIG // ge/rFlrR2Uu1WwtFmc1xGpy4+uxobCEMeIFDGhL5DNTb
// SIG // bOisLrBUYbyXr7fPzxbVkEwJDP5FG2n0ro1qOjegIkLI
// SIG // jXU6qahduQxTfsPOEp8jgqMKn++fpH6fvXKlewWzdsfv
// SIG // hiZ4H4Iq1CTOn+fkxqcDwTHYkYZYgqm+1X1x7458rp69
// SIG // qjFeVP3GbAvJbY3bFlq5uyxriPcZxDZrB6f1wALXrO2/
// SIG // IdfVEdwTWqJIDZBJjTycQhhxS3i1AgMBAAGjggEbMIIB
// SIG // FzAdBgNVHQ4EFgQUhzLwaZ8OBLRJH0s9E63pIcWJokcw
// SIG // HwYDVR0jBBgwFoAU1WM6XIoxkPNDe3xGG8UzaFqFbVUw
// SIG // VgYDVR0fBE8wTTBLoEmgR4ZFaHR0cDovL2NybC5taWNy
// SIG // b3NvZnQuY29tL3BraS9jcmwvcHJvZHVjdHMvTWljVGlt
// SIG // U3RhUENBXzIwMTAtMDctMDEuY3JsMFoGCCsGAQUFBwEB
// SIG // BE4wTDBKBggrBgEFBQcwAoY+aHR0cDovL3d3dy5taWNy
// SIG // b3NvZnQuY29tL3BraS9jZXJ0cy9NaWNUaW1TdGFQQ0Ff
// SIG // MjAxMC0wNy0wMS5jcnQwDAYDVR0TAQH/BAIwADATBgNV
// SIG // HSUEDDAKBggrBgEFBQcDCDANBgkqhkiG9w0BAQsFAAOC
// SIG // AQEAZhKWwbMnC9Qywcrlgs0qX9bhxiZGve+8JED27hOi
// SIG // yGa8R9nqzHg4+q6NKfYXfS62uMUJp2u+J7tINUTf/1ug
// SIG // L+K4RwsPVehDasSJJj+7boIxZP8AU/xQdVY7qgmQGmd4
// SIG // F+c5hkJJtl6NReYE908Q698qj1mDpr0Mx+4LhP/tTqL6
// SIG // HpZEURlhFOddnyLStVCFdfNI1yGHP9n0yN1KfhGEV3s7
// SIG // MBzpFJXwOflwgyE9cwQ8jjOTVpNRdCqL/P5ViCAo2dci
// SIG // Hjd1u1i1Q4QZ6xb0+B1HdZFRELOiFwf0sh3Z1xOeSFcH
// SIG // g0rLE+rseHz4QhvoEj7h9bD8VN7/HnCDwWpBJTCCBnEw
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
// SIG // As4wggI3AgEBMIH4oYHQpIHNMIHKMQswCQYDVQQGEwJV
// SIG // UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
// SIG // UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
// SIG // cmF0aW9uMSUwIwYDVQQLExxNaWNyb3NvZnQgQW1lcmlj
// SIG // YSBPcGVyYXRpb25zMSYwJAYDVQQLEx1UaGFsZXMgVFNT
// SIG // IEVTTjpBRTJDLUUzMkItMUFGQzElMCMGA1UEAxMcTWlj
// SIG // cm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaIjCgEBMAcG
// SIG // BSsOAwIaAxUAhyuClrocWf4SIcRafAEX1Rhs6zmggYMw
// SIG // gYCkfjB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQD
// SIG // Ex1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMDAN
// SIG // BgkqhkiG9w0BAQUFAAIFAORsnfQwIhgPMjAyMTA2MTAy
// SIG // MjE5MzJaGA8yMDIxMDYxMTIyMTkzMlowdzA9BgorBgEE
// SIG // AYRZCgQBMS8wLTAKAgUA5Gyd9AIBADAKAgEAAgIChQIB
// SIG // /zAHAgEAAgIRnTAKAgUA5G3vdAIBADA2BgorBgEEAYRZ
// SIG // CgQCMSgwJjAMBgorBgEEAYRZCgMCoAowCAIBAAIDB6Eg
// SIG // oQowCAIBAAIDAYagMA0GCSqGSIb3DQEBBQUAA4GBAANs
// SIG // 4cacsZtfhiNVTNk54J77MgAeiTQOWbBaGcrba5HgfYR/
// SIG // iMjHqAOGYrH03MsaTSn50NzBLI44Lz705z9IES6FmpNp
// SIG // f/jnt+8HvwXZ8+V9KJKHVJoddbIAHq8E7XKmRwM/XDGB
// SIG // tL0dwmwu2kskx7AU7866/AjyXIyTKKWnujw3MYIDDTCC
// SIG // AwkCAQEwgZMwfDELMAkGA1UEBhMCVVMxEzARBgNVBAgT
// SIG // Cldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAc
// SIG // BgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQG
// SIG // A1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIw
// SIG // MTACEzMAAAFIoohFVrwvgL8AAAAAAUgwDQYJYIZIAWUD
// SIG // BAIBBQCgggFKMBoGCSqGSIb3DQEJAzENBgsqhkiG9w0B
// SIG // CRABBDAvBgkqhkiG9w0BCQQxIgQg8VuMtJeUNSwKiAtM
// SIG // YoWaZOLNLoJUDnJ+XQhk8LwnVDwwgfoGCyqGSIb3DQEJ
// SIG // EAIvMYHqMIHnMIHkMIG9BCCpkBrqjHmhvyYf5tTcTvD5
// SIG // Y4a+V79TwVV6T1aAwdto2DCBmDCBgKR+MHwxCzAJBgNV
// SIG // BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
// SIG // VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
// SIG // Q29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBU
// SIG // aW1lLVN0YW1wIFBDQSAyMDEwAhMzAAABSKKIRVa8L4C/
// SIG // AAAAAAFIMCIEIL+zyxMd0ltIZvODF64+Q+0g+4eAEqIu
// SIG // +GVZZBX5FFGuMA0GCSqGSIb3DQEBCwUABIIBAC/Ow1iv
// SIG // E161moX3aVnjD9FHgRKQjFHcOYl+jY6w6rew99YXxC1Z
// SIG // E1YZ9mrSWB4FuSeP/lvBZWIO0nW69dEiUDPqVXumeqTE
// SIG // 2F5S7XO/+EhTVVQAI7hEBhmX7mYqsgJ9/e+0ESb2638c
// SIG // tfTfOScOdg3lISJiX1nodP3ETRdFffZzcnVMflbleNRR
// SIG // cdtVQDNWtxpuBcD7myQ9yHflCAUFdGACjo4sbmKfbm/4
// SIG // vH/7b5pngEsi5rCOAVq/GPW7ZhMJHNF3E/KutaPbE68e
// SIG // ESsW3hauz8ZFjKEKN4SOG7TiVhe+Nx7CW75ERR69fN9S
// SIG // 3h6HbyW6JblnzeaK/KO+LKzxNQM=
// SIG // End signature block
