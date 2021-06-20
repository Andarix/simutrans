<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
	<xsl:template match="/">
		<h1 _locID="L_string01_Text">Web Services on the Local Machine</h1>
		<p tabIndex="1" _locID="L_string02_Text">The Web services and Discovery Documents available on your VS.NET developer
			machine are listed below. Click the service link to browse that service.</p>
		<table class="listpage" cellpadding="3" cellspacing="1" frame="void" bordercolor="#ffffff" rules="rows" width="100%" align="center">
    	    <xsl:choose>
				<xsl:when test="localDiscovery/contractRef">
					<tr valign="center" align="left">
						<td class="header" width="125" _locID="L_string03_Text" nowrap="true">Services</td>
						<td class="header" id="120">URL</td>
					</tr>
					<xsl:for-each select="localDiscovery/contractRef" order-by='@ref'>
						<tr valign="center" align="left">
							<td class="tbltext" tabIndex="2">
								<a _locID="L_string04_Text"><xsl:attribute name="href"><xsl:value-of select="@ref" /></xsl:attribute><xsl:value-of select="@name" /></a>
							</td>
							<td class="tbltext" tabIndex="3" nowrap="true">
								<xsl:value-of select="@ref" />
							</td>
						</tr>
					</xsl:for-each>
				</xsl:when>
			</xsl:choose>
			<xsl:choose>
				<xsl:when test="localDiscovery/discoveryRef">
					<tr valign="center" align="left">
						<td class="header" _locID="L_string05_Text">Discovery Documents</td>
						<td class="header" _locID="L_string06_Text">URL</td>
					</tr>
					<xsl:for-each select="localDiscovery/discoveryRef" order-by='@ref'>
						<tr valign="center" align="left">
							<td class="tbltext" tabIndex="4">
								<a _locID="L_string07_Text"><xsl:attribute name="href"><xsl:value-of select="@ref" /></xsl:attribute><xsl:value-of select="@name" /></a>
							</td>
							<td class="tbltext" tabIndex="5" nowrap="true">
								<xsl:value-of select="@ref" />
							</td>
						</tr>
					</xsl:for-each>
				</xsl:when>
			</xsl:choose>
			<xsl:choose>
				<xsl:when test="discoveryError">
            		<tr valign="center" align="left">
    					<tr>
    						<td class="tbltext" tabIndex="6" colspan="2" _locID="L_string09_Text">There was an error while enumerating services on local machine: </td>
    					</tr>
            		</tr>
					<xsl:for-each select="discoveryError">
						<tr valign="center" align="left">
							<td class="tbltext" tabIndex="7">
								<xsl:value-of select="@errorMessage" />
							</td>
						</tr>
					</xsl:for-each>
				</xsl:when>
			</xsl:choose>

			<xsl:choose>
				<xsl:when test="localDiscovery/contractRef | localDiscovery/discoveryRef | discoveryError"></xsl:when>
				<xsl:otherwise>
					<tr>
						<td class="tbltext" tabIndex="8" colspan="2" _locID="L_string08_Text">None - No Web services were found on the local computer.</td>
					</tr>
				</xsl:otherwise>
			</xsl:choose>
		</table>
	</xsl:template>
</xsl:stylesheet>
