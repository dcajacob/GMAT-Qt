<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:d="http://docbook.org/ns/docbook"
                xmlns:xlink="http://www.w3.org/1999/xlink"
                version="1.0"
                exclude-result-prefixes="d xlink">
  <xsl:import href="../../build/contrib/docbook-xsl-ns-1.78.1/html/chunkfast.xsl"/>

  <!-- Chunk pages live in html/; attachments live in the sibling files/.
       Keep source URIs relative to help.html for the single-page build. -->
  <xsl:template match="d:link[starts-with(@xlink:href, 'files/')]">
    <xsl:call-template name="link">
      <xsl:with-param name="xhref" select="concat('../', @xlink:href)"/>
    </xsl:call-template>
  </xsl:template>
</xsl:stylesheet>
