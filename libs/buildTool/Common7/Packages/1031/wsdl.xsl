<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/"
    xmlns:ms="urn:schemas-microsoft-com:xslt" xmlns:s="http://www.w3.org/2001/XMLSchema" exclude-result-prefixes="wsdl ms s">
    <xsl:output method="html" />

<xsl:template match="definitions">
    <xsl:choose>
        <xsl:when test="service | portType">
        <xsl:choose>
            <xsl:when test="service">
                <xsl:for-each select="service">
                    <xsl:sort select="@name" />
                    <h1 class="listpage" _locID="L_string01_Text">"<xsl:value-of select="@name" />" Description</h1>

                    <xsl:if test="documentation">
                        <h2 _locID="L_string02_Text">Documentation</h2>
                        <p>
                            <xsl:value-of select="."/>
                        </p>

                    </xsl:if>
                </xsl:for-each>
            </xsl:when>
            <xsl:otherwise>
                <xsl:if test="documentation">
                    <h1 class="listpage" _locID="L_string03_Text">Description</h1>
                    <h2 _locID="L_string12_Text">Documentation</h2>
                    <p>
                        <xsl:value-of select="."/>
                    </p>

                </xsl:if>
            </xsl:otherwise>
        </xsl:choose>
        <!-- walk the methods in the binding operation elements -->
        <xsl:choose>
            <xsl:when test="portType">
                <p></p>

                <h2 _locID="L_string04_Text">Methods</h2>

                <!-- walk the bindings (protocols) -->
                <xsl:variable name="varMethods">
                    <xsl:copy-of select="//operation" />
                </xsl:variable>
                <!-- Just saving the root -->
                <xsl:variable name="root" select="/" />
                <xsl:for-each select="ms:node-set($varMethods)//operation">
                    <xsl:sort select="@name" />
                    <xsl:if test="not(preceding-sibling::*[@name=current()/@name])">
                        <ul class="wsdl">
                            <li class="wsdl">
                                <span class="method">
                                    <span class="method_name">
                                        <xsl:value-of select="@name" />
                                    </span>
                                    <!-- show param info if params exist -->
                                    (
                                    <xsl:if test="current()/input/@message">
                                        <xsl:for-each select="$root/wsdl/definitions/types/schema/element[@name=$root/wsdl/definitions/message[@name=current()/input/@message]/@element]">
                                            <xsl:if test="position()=1">
                                                <xsl:choose>
                                                    <xsl:when test="complexType/sequence">
                                                        <xsl:for-each select="complexType/sequence/element">
                                                            <xsl:if test="not(position()=1)">
                                                                ,&#160;
                                                            </xsl:if>
                                                            <span class="method_param">
                                                                <xsl:value-of select="@name"/>
                                                            </span>
                                                            As
                                                            <span class="method_type">
                                                                <xsl:value-of select="@type"/>
                                                            </span>
                                                        </xsl:for-each>
                                                    </xsl:when>

                                                    <xsl:when test="@type">
                                                        <span class="method_param">
                                                            <xsl:value-of select="@name"/>
                                                        </span>
                                                        As
                                                        <span class="method_type">
                                                            <xsl:value-of select="@type"/>
                                                        </span>
                                                    </xsl:when>

                                                </xsl:choose>

                                            </xsl:if>
                                        </xsl:for-each>

                                    </xsl:if>
                                    )
                                    <!-- show return type if this is not a void function -->


                                    <xsl:if test="current()/output/@message">
                                        <xsl:for-each select="$root/wsdl/definitions/types/schema/element[@name=$root/wsdl/definitions/message[@name=current()/output/@message]/@element]">
                                            <xsl:if test="position()=1">
                                                <xsl:choose>
                                                    <xsl:when test="complexType/sequence/element/@type">
                                                        As
                                                        <span class="method_type">
                                                            <xsl:value-of select="complexType/sequence/element/@type"/>
                                                        </span>
                                                    </xsl:when>

                                                    <xsl:when test="@type">
                                                        As
                                                        <span class="method_type">
                                                            <xsl:value-of select="@type"/>
                                                        </span>
                                                    </xsl:when>
                                                </xsl:choose>
                                            </xsl:if>
                                        </xsl:for-each>
                                    </xsl:if>
                                </span>
                                <br />
                                <xsl:if test="documentation">
                                    <span class="methoddescription">
                                        <xsl:value-of select="." />
                                        <p />
                                    </span>
                                </xsl:if>
                            </li>
                        </ul>
                    </xsl:if>
                </xsl:for-each>
            </xsl:when>
            <xsl:otherwise>
                <br/><p class="heading2" _locID="L_string05_Text">No Ports or Methods were found on this page.</p>

                <p _locID="L_string06_Text">If this is an ASP.NET Web service, make sure that all WebMethods are public and
                    have a &lt;WebMethod&gt; attribute.
                </p>
                <p _locID="L_string07_Text">Visual Basic example:</p>
                <BLOCKQUOTE dir="ltr" style="MARGIN-RIGHT: 0px">
                    <p _locID="L_string08_Text">&lt;WebMethod()&gt; _<br />
                Public Function HelloWorld() as String<br />
                &#160;&#160;&#160;Return "Hello World!"<br />
                End Sub</p>
                </BLOCKQUOTE>
                <p dir="ltr" _locID="L_string09_Text">Visual C# example:</p>
                <BLOCKQUOTE dir="ltr" style="MARGIN-RIGHT: 0px">
                    <p dir="ltr" _locID="L_string10_Text">[WebMethod]<br />
                public string HelloWorld() {<br />
                   &#160;&#160;&#160;return "Hello World!";<br />
                }</p>
                </BLOCKQUOTE>

            </xsl:otherwise>
        </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
            <p class="heading2" _locID="L_string11_Text">No Web Services were found on this page.</p>
    </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template match="text()" />
</xsl:stylesheet>
